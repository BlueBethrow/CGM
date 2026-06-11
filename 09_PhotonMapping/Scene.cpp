#include "Scene.h"
#include "Material.h"
#include "RectAreaLight.h"
#include "Sphere.h"
#include "Plane.h"
#include "Rectangle.h"

namespace {

Vec3 transformDirection(const Mat4& matrix, const Vec3& direction)
{
	return (matrix * Vec4{direction, 0.0f}).xyz;
}

Vec3 bilerp(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, float u, float v) {
	const Vec3 bottom = a * (1.0f - u) + b * u;
	const Vec3 top = d * (1.0f - u) + c * u;
	return bottom * (1.0f - v) + top * v;
}

void addSubdividedRectangle(Scene& scene,
                            const Vec3& a,
                            const Vec3& b,
                            const Vec3& c,
                            const Vec3& d,
                            const Material& material,
                            size_t uSteps,
                            size_t vSteps) {
	for (size_t v = 0; v < vSteps; ++v) {
		const float v0 = float(v) / float(vSteps);
		const float v1 = float(v + 1) / float(vSteps);
		for (size_t u = 0; u < uSteps; ++u) {
			const float u0 = float(u) / float(uSteps);
			const float u1 = float(u + 1) / float(uSteps);
			scene.addObject(std::make_shared<Rectangle>(bilerp(a, b, c, d, u0, v0),
			                                            bilerp(a, b, c, d, u1, v0),
			                                            bilerp(a, b, c, d, u1, v1),
			                                            bilerp(a, b, c, d, u0, v1),
			                                            material));
		}
	}
}

} // namespace

void Scene::addObject(std::shared_ptr<const IntersectableObject> object) {
	sceneObjects.push_back(object);
}

void Scene::addLight(std::shared_ptr<const LightSource> ls) {
	lightSources.push_back(ls);
}

std::shared_ptr<const LightSource> Scene::getLight(size_t index) const {
	if (index >= lightSources.size())
		return {};

	return lightSources[index];
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
		const Material material = object->getMaterial();
		const Vec3 color = material.emits() ? Vec3::clamp(material.getEmission(), 0.0f, 1.0f) : material.getDiffuse();
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

std::optional<Intersection> Scene::intersect(const Ray& ray,
                                             bool shadowRay) const {
	std::optional<Intersection> result{};
	for (std::shared_ptr<const IntersectableObject> object : sceneObjects)
	{
		if (shadowRay && !object->getMaterial().isShadowCaster())
			continue;

		std::optional<Intersection> i = object->intersect(ray);
		if (!i)
			continue;

		if (!result || i.value().getT() < result.value().getT())
			result = i;
	}
	return result;
}

/// <summary>
/// Trace a ray through the scene and compute its color value.
/// </summary>
/// <param name="ray">to trace</param>
/// <param name="IOR">optical density of the material we are currently travelling in</param>
/// <param name="recDepth">recursion depth</param>
/// <returns>final color value computed for this ray</returns>
Vec3 Scene::traceRay(const Ray& ray, float IOR, int recDepth) const {
	const Mat4 inverseModel = Mat4::inverse(model);
	const Vec3 localDirection = Vec3::normalize(transformDirection(inverseModel, ray.getDirection()));
	if (localDirection.sqlength() == 0.0f)
		return backgroundColor;

	const Ray localRay{ inverseModel * ray.getOrigin(), localDirection };
	return traceLocalRay(localRay, IOR, recDepth);
}

Vec3 Scene::traceLocalRay(const Ray& ray, float IOR, int recDepth) const {
	if (recDepth == 0) return backgroundColor;

	// no intersection found
	std::optional<Intersection> opt_intersection = intersect(ray, false);
	if (!opt_intersection.has_value()) return backgroundColor;

	// else intersection found, do recursive ray tracing
	Intersection inter = opt_intersection.value();
	Vec3 interPos = ray.getPosOnRay(inter.getT());
	if (inter.getMaterial().emits())
		return inter.getMaterial().getEmission();

  Vec3 reflColor{ 0.0f, 0.0f, 0.0f };
  if (inter.getMaterial().reflects()) {
    Vec3 reflDir = Vec3::reflect(ray.getDirection(), inter.getNormal());
    Vec3 reflOrigin = interPos + inter.getNormal() * (Vec3::dot(reflDir, inter.getNormal()) > 0 ? OFFSET_EPSILON : -OFFSET_EPSILON);
    Ray reflRay{reflOrigin, reflDir};
    reflColor = traceLocalRay(reflRay, IOR, recDepth - 1);
  }

  Vec3 refractionColor{ 0.0f, 0.0f, 0.0f };
  if (inter.getMaterial().refracts()) {
    const float matIOR = *inter.getMaterial().getIndexOfRefraction();
    std::optional<Vec3> potentialRefDir = Vec3::refract(ray.getDirection(), inter.getNormal(), matIOR);
    if (potentialRefDir) {
      const Vec3 refDir = *potentialRefDir;
      if (Vec3::dot(refDir, inter.getNormal()) > 0) {
        // Ray --> from material into air
        Ray refrRay{ interPos + inter.getNormal() * OFFSET_EPSILON, refDir };
        refractionColor = traceLocalRay(refrRay, 1.0, recDepth - 1);
      } else {
        // Ray --> from air into material
        Vec3 inSurfacePos = interPos + inter.getNormal() * -OFFSET_EPSILON;
        Ray refrRay{ inSurfacePos, refDir };
        refractionColor = traceLocalRay(refrRay, matIOR, recDepth - 1);
      }
    } else {
      // Total internal reflection
    }
  }

	Vec3 localColor{ 0.0f, 0.0f, 0.0f };

  const Vec3 offSurfacePos = interPos + inter.getNormal() * OFFSET_EPSILON;
	for (const std::shared_ptr<const LightSource>& ls : lightSources) {
		Ray shadowRay{ offSurfacePos, ls->getDirection(offSurfacePos) };
		std::optional<Intersection> shadowInter = intersect(shadowRay, true);

		Vec3 ambient = inter.getMaterial().getAmbient() * ls->getAmbient();

		if (!shadowInter || shadowInter->getT() > ls->getDistance(offSurfacePos)) {
			float d = Vec3::dot(ls->getDirection(offSurfacePos), inter.getNormal());
			Vec3 diffuse = inter.getMaterial().getDiffuse() * ls->getDiffuse() * d;
			diffuse = Vec3::clamp(diffuse, 0.0f, 1.0f);

			Vec3 Rv = Vec3::reflect(ray.getDirection(), inter.getNormal());
			float s = pow(std::max(0.0f, Vec3::dot(Rv, ls->getDirection(offSurfacePos))), inter.getMaterial().getExp());
			Vec3 specular = inter.getMaterial().getSpecular() * ls->getSpecular() * s;
			specular = Vec3::clamp(specular, 0.0f, 1.0f);

			localColor = localColor + ambient + diffuse + specular;
		} else {
			localColor = localColor + ambient;
		}
	}

	// compose final color
	float cosI = Vec3::dot(ray.getDirection(), inter.getNormal());
	float l = 0, r = 0, t = 0;
	if (inter.getMaterial().refracts()) {
		l = inter.getMaterial().getLocalRefectivity();
		r = inter.getMaterial().getReflectivity(cosI);
		t = 1 - r;
		r = (1 - l) * r;
		t = (1 - l) * t;
	} else if (inter.getMaterial().reflects()) {
		r = inter.getMaterial().getReflectivity(cosI);
		l = 1 - r;
	} else {
		l = 1;
	}
	return localColor * l + reflColor * r + refractionColor * t;
}

Scene Scene::genCornellBox() {
	Scene scene{Vec3{0.01f, 0.01f, 0.012f}};

	const Material whiteDiffuse{Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.82f, 0.82f, 0.78f}, Vec3{0.0f, 0.0f, 0.0f}, 1.0f};
	const Material redDiffuse{Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.85f, 0.06f, 0.03f}, Vec3{0.0f, 0.0f, 0.0f}, 1.0f};
	const Material blueDiffuse{Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.06f, 0.12f, 0.88f}, Vec3{0.0f, 0.0f, 0.0f}, 1.0f};
	const Material silverMirror{Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.72f, 0.72f, 0.72f}, Vec3{0.95f, 0.95f, 0.92f}, 96.0f, 0.0f};
	const Material glass{Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.72f, 0.88f, 1.0f}, Vec3{1.0f, 1.0f, 1.0f}, 96.0f, 0.0f, 1.52f};
	const Material lightPanel{Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 0.95f, 0.82f}, Vec3{0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, std::nullopt, false, Vec3{1.0f, 0.95f, 0.82f}};

	std::shared_ptr<const RectAreaLight> light = std::make_shared<const RectAreaLight>(
		Vec3{-0.45f, 1.34f, -2.80f},
		Vec3{0.45f, 1.34f, -2.80f},
		Vec3{-0.45f, 1.34f, -1.90f},
		Vec3{0.0f, 0.0f, 0.0f},
		Vec3{1.0f, 0.95f, 0.82f},
		Vec3{0.0f, 0.0f, 0.0f});
	scene.addLight(light);
	scene.addObject(std::make_shared<Rectangle>(
		Vec3{-0.45f, 1.34f, -2.80f},
		Vec3{0.45f, 1.34f, -2.80f},
		Vec3{0.45f, 1.34f, -1.90f},
		Vec3{-0.45f, 1.34f, -1.90f},
		lightPanel));

	const float x0 = -1.65f;
	const float x1 = 1.65f;
	const float y0 = -1.25f;
	const float y1 = 1.45f;
	const float z0 = -4.2f;
	const float z1 = -0.7f;
	const size_t wallSteps = 2;

	addSubdividedRectangle(scene, Vec3{x0, y0, z1}, Vec3{x1, y0, z1}, Vec3{x1, y0, z0}, Vec3{x0, y0, z0}, whiteDiffuse, wallSteps, wallSteps);
	addSubdividedRectangle(scene, Vec3{x0, y1, z0}, Vec3{x1, y1, z0}, Vec3{x1, y1, z1}, Vec3{x0, y1, z1}, whiteDiffuse, wallSteps, wallSteps);
	addSubdividedRectangle(scene, Vec3{x0, y0, z0}, Vec3{x1, y0, z0}, Vec3{x1, y1, z0}, Vec3{x0, y1, z0}, whiteDiffuse, wallSteps, wallSteps);
	addSubdividedRectangle(scene, Vec3{x0, y0, z1}, Vec3{x0, y0, z0}, Vec3{x0, y1, z0}, Vec3{x0, y1, z1}, redDiffuse, wallSteps, wallSteps);
	addSubdividedRectangle(scene, Vec3{x1, y0, z0}, Vec3{x1, y0, z1}, Vec3{x1, y1, z1}, Vec3{x1, y1, z0}, blueDiffuse, wallSteps, wallSteps);

	scene.addObject(std::make_shared<Sphere>(Vec3{-0.55f, -0.72f, -2.15f}, 0.48f, silverMirror));
	scene.addObject(std::make_shared<Sphere>(Vec3{0.55f, -0.78f, -1.70f}, 0.42f, glass));

	return scene;
}
