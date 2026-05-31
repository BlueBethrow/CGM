#include "RectAreaLight.h"

RectAreaLight::RectAreaLight(const Vec3& a,
                             const Vec3& b,
                             const Vec3& d,
                             const Vec3& ambient,
                             const Vec3& diffuse,
                             const Vec3& specular)
	: LightSource(ambient, diffuse, specular)
	, a(a)
	, edgeU(b - a)
	, edgeV(d - a)
	, center(a + (b - a) * 0.5f + (d - a) * 0.5f)
	, normal(Vec3::normalize(Vec3::cross(b - a, d - a)))
{
}

Vec3 RectAreaLight::getDirection(const Vec3& position) const {
	return Vec3::normalize(center - position);
}

float RectAreaLight::getDistance(const Vec3& position) const {
	return (center - position).length();
}

Vec3 RectAreaLight::samplePosition() const {
	const Vec3 random = Vec3::random();
	return a + edgeU * random.x + edgeV * random.y;
}

Vec3 RectAreaLight::sampleEmissionDirection() const {
	Vec3 direction = Vec3::randomUnitVector();
	if (Vec3::dot(direction, normal) < 0.0f)
		direction = direction * -1.0f;

	return direction;
}

Vec3 RectAreaLight::getCenter() const {
	return center;
}
