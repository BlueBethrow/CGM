#include "Sphere.h"
#include <cmath>

#ifndef M_PI
constexpr float M_PI = 3.14159265358979323846f;
#endif

Sphere::Sphere(const Vec3& center, float radius, const Material& material) :
  center(center),
  sqradius(radius*radius),
  material(material)
{
}

Material Sphere::getMaterial() const {
	return material;
}

std::optional<Intersection> Sphere::intersect(const Ray& ray) const {
  const Vec3 l = center - ray.getOrigin();
  const float tCenter = Vec3::dot(l, ray.getDirection());
	if (tCenter < 0) return {};	// no intersection

  const float dSq = l.sqlength() - tCenter * tCenter;
	if (dSq > sqradius) return {};	// no intersection

	const float dist = sqrt(sqradius - dSq);
  float t = tCenter - dist;

	if (t < 0) t = tCenter + dist;	// when inside sphere

	const Vec3 normal = Vec3::normalize(ray.getPosOnRay(t) - center);
  return Intersection{material, normal, t};
}

Tessellation Sphere::getMesh() const
{
	return Tessellation::genSphere(center, std::sqrt(sqradius), 32, 16);
}
