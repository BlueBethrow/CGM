#pragma once

#include "IntersectableObject.h"

class Rectangle : public IntersectableObject {
private:
	Vec3 a;
	Vec3 b;
	Vec3 c;
	Vec3 d;
	Vec3 normal;
	Material material;

public:
	Rectangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Material& material);

	Material getMaterial() const override;
	std::optional<Intersection> intersect(const Ray& ray) const override;
	Tessellation getMesh() const override;
};
