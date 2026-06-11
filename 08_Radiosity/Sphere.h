#pragma once

#include "IntersectableObject.h"

#include <cstddef>

class Sphere : public IntersectableObject {
private:
	const Vec3 center;
	const float sqradius;
	const Material material;
	const size_t tessellationFactor;

public:
	Sphere(const Vec3& center, float radius, const Material& material, size_t tessellationFactor = 4);

	Material getMaterial() const override;
	std::optional<Intersection> intersect(const Ray& ray) const override;
	Tessellation getMesh() const override;
};
