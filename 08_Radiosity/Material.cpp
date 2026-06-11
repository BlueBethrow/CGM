#include "Material.h"

Vec3 Material::getReflectance() const {
	return reflectance;
}

Vec3 Material::getEmission() const {
	return emission;
}

bool Material::emits() const {
	return emission.sqlength() > 0.0f;
}
