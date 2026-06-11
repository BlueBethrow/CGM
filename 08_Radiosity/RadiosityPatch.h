#pragma once

#include <Vec3.h>

struct RadiosityPatch {
	// Geometry of one triangular patch.
	Vec3 a;
	Vec3 b;
	Vec3 c;
	Vec3 center;
	Vec3 normal;
	float area{0.0f};

	// Diffuse material and emitted light.
	Vec3 reflectance;
	Vec3 emission;

	// Solved outgoing light for this patch.
	Vec3 radiosity;
};
