#pragma once

#include "LightSource.h"

class RectAreaLight : public LightSource {
private:
	Vec3 a;
	Vec3 edgeU;
	Vec3 edgeV;
	Vec3 center;
	Vec3 normal;

public:
	RectAreaLight(const Vec3& a,
	              const Vec3& b,
	              const Vec3& d,
	              const Vec3& ambient,
	              const Vec3& diffuse,
	              const Vec3& specular);

	Vec3 getDirection(const Vec3& position) const override;
	float getDistance(const Vec3& position) const override;

	Vec3 samplePosition() const;
	Vec3 sampleEmissionDirection() const;
	Vec3 getCenter() const;
};
