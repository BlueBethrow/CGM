#include "Scene.h"
#include "Material.h"
#include "Sphere.h"
#include "Plane.h"
#include "Texture.h"

#include <algorithm>
#include <cmath>

namespace {

Vec3 transformDirection(const Mat4& matrix, const Vec3& direction) {
	return (matrix * Vec4{direction, 0.0f}).xyz;
}

[[maybe_unused]] Vec3 sampleDiffuseDirection(const Vec3& normal) {
	// TODO: Generate one random outgoing direction for a perfectly diffuse
	// material.
	//
	// A diffuse surface scatters light over the hemisphere above the surface
	// instead of reflecting into one deterministic mirror direction. For this
	// exercise, use the simple cosine-weighted construction from the lecture:
	//
	//   1. Generate a random unit vector with Vec3::randomUnitVector().
	//   2. Add the surface normal to shift the random vector into the hemisphere
	//      around the normal.
	//   3. If the result is almost zero, fall back to the normal itself. This can
	//      happen if the random vector is almost exactly opposite to the normal.
	//   4. Normalize and return the resulting direction.
	//
	// The returned direction will later be used as the next ray direction of the
	// path. Averaging many such paths approximates the diffuse lighting integral.
	return normal;
}

[[maybe_unused]] Vec3 offsetRayOrigin(const Vec3& position, const Vec3& normal, const Vec3& direction) {
	constexpr float offsetEpsilon = 0.00001f;
	const float side = Vec3::dot(direction, normal) > 0.0f ? 1.0f : -1.0f;
	return position + normal * (offsetEpsilon * side);
}

[[maybe_unused]] Vec3 getAlbedo(const Intersection& inter)
{
	Vec3 albedo = inter.getMaterial().getDiffuse();
	if (inter.getMaterial().hasTexture() && inter.getTexCoords().has_value())
		albedo = albedo * inter.getMaterial().getTexture().value().sample(inter.getTexCoords().value());

	return albedo;
}

[[maybe_unused]] Vec3 environmentColor(const Vec3& direction)
{
	const Vec3 dir = Vec3::normalize(direction);
	const Vec3 horizon{0.45f, 0.48f, 0.55f};
	const Vec3 zenith{1.15f, 1.25f, 1.55f};
	const Vec3 ground{0.32f, 0.31f, 0.28f};

	if (dir.y > 0.0f)
	{
		const float t = std::sqrt(std::clamp(dir.y, 0.0f, 1.0f));
		const float zenithSpot = std::pow(std::clamp(dir.y, 0.0f, 1.0f), 40.0f);
		return horizon * (1.0f - t) + zenith * t + Vec3{25.0f, 24.0f, 20.0f} * zenithSpot;
	}

	const float t = std::clamp(-dir.y, 0.0f, 1.0f);
	return horizon * (1.0f - t) + ground * t;
}

} // namespace

void Scene::addObject(std::shared_ptr<const IntersectableObject> object) {
	sceneObjects.push_back(object);
}

void Scene::setModel(const Mat4& model) {
	// This local-space raytracing path assumes translations, rotations, and uniform scales only.
	this->model = model;
}

Mat4 Scene::getModel() const {
	return model;
}

Vec3 Scene::getBackgroundcolor() const {
	return backgroundColor;
}

std::vector<float> Scene::getTriangleData() const {
	std::vector<float> data;

	for (std::shared_ptr<const IntersectableObject> object : sceneObjects)
	{
		const Tessellation mesh = object->getMesh().unpack();
		const std::vector<float>& vertices = mesh.getVertices();
		const std::vector<float>& normals = mesh.getNormals();
		const Vec3 color = object->getMaterial().getDiffuse();
		const size_t vertexCount = vertices.size() / 3;

		data.reserve(data.size() + vertexCount * 10);
		for (size_t i = 0; i < vertexCount; ++i)
		{
			data.push_back(vertices[i * 3 + 0]);
			data.push_back(vertices[i * 3 + 1]);
			data.push_back(vertices[i * 3 + 2]);
			data.push_back(color.r);
			data.push_back(color.g);
			data.push_back(color.b);
			data.push_back(1.0f);

			if (normals.size() >= (i + 1) * 3)
			{
				data.push_back(normals[i * 3 + 0]);
				data.push_back(normals[i * 3 + 1]);
				data.push_back(normals[i * 3 + 2]);
			}
			else
			{
				data.push_back(0.0f);
				data.push_back(0.0f);
				data.push_back(1.0f);
			}
		}
	}

	return data;
}

std::optional<Intersection> Scene::intersect(const Ray& ray) const {
	std::optional<Intersection> result{};
	for (std::shared_ptr<const IntersectableObject> object : sceneObjects)
	{
		std::optional<Intersection> i = object->intersect(ray);
		if (!i)
			continue;

		if (!result || i.value().getT() < result.value().getT())
			result = i;
	}
	return result;
}

Vec3 Scene::tracePath(const Ray& ray, int maxDepth) const {
	const Mat4 inverseModel = Mat4::inverse(model);
	const Vec3 localDirection = Vec3::normalize(transformDirection(inverseModel, ray.getDirection()));
	if (localDirection.sqlength() == 0.0f)
		return backgroundColor;

	const Ray localRay{ inverseModel * ray.getOrigin(), localDirection };
	return traceLocalPath(localRay, maxDepth);
}

Vec3 Scene::traceLocalPath(const Ray& firstRay, int maxDepth) const {
	Ray ray = firstRay;

	Vec3 throughput{1.0f, 1.0f, 1.0f};

	for (int depth = 0; depth < maxDepth; ++depth)
	{
		// TODO: Trace one path bounce.
		//
		// 1. Intersect the current ray with the scene, just as in the raytracer.
		//
		// 2. If the ray misses all objects, it has reached the environment light.
		//    Return the environment color multiplied by the accumulated throughput:
		//
		//       return throughput * environmentColor(transformDirection(model, ray.getDirection()));
		//
		// 3. If the ray hits an object, compute the hit position, normal, material,
		//    and the next ray direction.
		//
		// 4. For refractive materials, reuse the refraction code from the texturing
		//    raytracer:
		//      - compute the Fresnel reflectivity with material.getReflectivity(...)
		//      - compute the refracted direction with Vec3::refract(...)
		//      - use the Fresnel value as a probability to choose reflection or
		//        refraction for this one path
		//      - if refraction fails, use reflection
		//      - multiply throughput by material.getSpecular() * getAlbedo(inter)
		//
		// 5. For mirror materials, reuse Vec3::reflect(...) from the raytracer and
		//    multiply throughput by material.getSpecular() * getAlbedo(inter).
		//
		// 6. For diffuse materials, call sampleDiffuseDirection(...). Multiply the
		//    throughput by getAlbedo(inter). This replaces the old Phong light loop:
		//    instead of evaluating all lights at this surface, the path randomly
		//    continues and gathers light when it eventually reaches the environment.
		//
		// 7. Normalize the next direction and continue the path with a new ray from
		//    offsetRayOrigin(...), so the new ray does not immediately hit the same
		//    surface again.
	}


	return Vec3{0.0f, 0.0f, 0.0f};
}

Scene Scene::genPathTracingScene() {
	Scene s{Vec3{0.5f, 0.5f, 0.5f}};

	Texture checkerboard = Texture::genCheckerboardTexture(2, 2);
	Texture earth("Earth.png");

	Material glass(Vec3{}, Vec3{0.35f, 0.55f, 1.0f}, Vec3{0.65f, 0.78f, 1.0f}, 1.0f, 0.0f, 1.52f);
	s.addObject(std::make_shared<Sphere>(Vec3{0.7f, -0.4f, -2.0f}, 0.9f, glass));

	Material earthMaterial(Vec3{}, Vec3{1.0f, 1.0f, 1.0f}, Vec3{}, 1.0f, 1.0f, {}, earth);
	s.addObject(std::make_shared<Sphere>(Vec3{-0.9f, -0.1f, -2.2f}, 0.6f, earthMaterial, Vec3{-90.0f, 0.0f, -90.0f}));

	Material redDiffuse(Vec3{}, Vec3{0.95f, 0.08f, 0.04f}, Vec3{}, 1.0f, 1.0f, {});
	s.addObject(std::make_shared<Sphere>(Vec3{2.15f, -0.95f, -3.35f}, 0.55f, redDiffuse));

	Material mirror(Vec3{}, Vec3{1.0f, 0.9f, 0.1f}, Vec3{1.0f, 0.85f, 0.15f}, 1.0f, 0.0f, {});
	s.addObject(std::make_shared<Sphere>(Vec3{0.0f, 4.0f, -8.0f}, 3.9f, mirror, Vec3{-60.0f, 0.0f, -90.0f}));

	Material floor(Vec3{}, Vec3{0.75f, 0.75f, 0.75f}, Vec3{}, 1.0f, 1.0f, {}, checkerboard);
	s.addObject(std::make_shared<Plane>(Vec3{0.0f, 1.0f, 0.0f}, 1.5f, floor));

	return s;
}
