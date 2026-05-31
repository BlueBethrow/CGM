#include "Rectangle.h"

#include <algorithm>
#include <cmath>

namespace {

bool pointInTriangle(const Vec3& point, const Vec3& a, const Vec3& b, const Vec3& c) {
	const Vec3 v0 = c - a;
	const Vec3 v1 = b - a;
	const Vec3 v2 = point - a;

	const float dot00 = Vec3::dot(v0, v0);
	const float dot01 = Vec3::dot(v0, v1);
	const float dot02 = Vec3::dot(v0, v2);
	const float dot11 = Vec3::dot(v1, v1);
	const float dot12 = Vec3::dot(v1, v2);
	const float denominator = dot00 * dot11 - dot01 * dot01;
	if (std::fabs(denominator) < 0.000001f)
		return false;

	const float invDenominator = 1.0f / denominator;
	const float u = (dot11 * dot02 - dot01 * dot12) * invDenominator;
	const float v = (dot00 * dot12 - dot01 * dot02) * invDenominator;
	return u >= 0.0f && v >= 0.0f && u + v <= 1.0f;
}

} // namespace

Rectangle::Rectangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Material& material)
	: a(a)
	, b(b)
	, c(c)
	, d(d)
	, normal(Vec3::normalize(Vec3::cross(b - a, c - a)))
	, material(material)
{
}

Material Rectangle::getMaterial() const {
	return material;
}

std::optional<Intersection> Rectangle::intersect(const Ray& ray) const {
	const float denominator = Vec3::dot(ray.getDirection(), normal);
	if (std::fabs(denominator) < 0.000001f)
		return {};

	const float t = Vec3::dot(a - ray.getOrigin(), normal) / denominator;
	if (t < 0.0f)
		return {};

	const Vec3 point = ray.getPosOnRay(t);
	if (!pointInTriangle(point, a, b, c) && !pointInTriangle(point, a, c, d))
		return {};

	const Vec3 hitNormal = denominator < 0.0f ? normal : normal * -1.0f;
	return Intersection{material, hitNormal, t};
}

Tessellation Rectangle::getMesh() const {
	return Tessellation::genRectangle(a, b, c, d);
}
