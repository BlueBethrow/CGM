#include "Sphere.h"

#include <algorithm>
#include <cmath>

Sphere::Sphere(const Vec3& center, float radius, const Material& material, size_t tessellationFactor)
	: center(center)
	, sqradius(radius * radius)
	, material(material)
	, tessellationFactor(tessellationFactor)
{
}

Material Sphere::getMaterial() const {
	return material;
}

std::optional<Intersection> Sphere::intersect(const Ray& ray) const {
	const Vec3 l = center - ray.getOrigin();
	const float tCenter = Vec3::dot(l, ray.getDirection());
	if (tCenter < 0.0f)
		return {};

	const float dSq = l.sqlength() - tCenter * tCenter;
	if (dSq > sqradius)
		return {};

	const float dist = std::sqrt(sqradius - dSq);
	float t = tCenter - dist;
	if (t < 0.0f)
		t = tCenter + dist;

	const Vec3 normal = Vec3::normalize(ray.getPosOnRay(t) - center);
	return Intersection{material, normal, t};
}

Tessellation Sphere::getMesh() const {
	const uint32_t sectorCount = uint32_t(std::max<size_t>(3, tessellationFactor * 4));
	const uint32_t stackCount = uint32_t(std::max<size_t>(2, tessellationFactor * 2));
	return Tessellation::genSphere(center, std::sqrt(sqradius), sectorCount, stackCount);
}
