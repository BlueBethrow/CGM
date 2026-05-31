#pragma once

#include <Vec3.h>

class Material {
private:
	Vec3 reflectance;
	Vec3 emission;

public:
	Material(const Vec3& reflectance = Vec3{}, const Vec3& emission = Vec3{})
		: reflectance(reflectance)
		, emission(emission)
	{
	}

	Vec3 getReflectance() const;
	Vec3 getEmission() const;
	bool emits() const;
};
