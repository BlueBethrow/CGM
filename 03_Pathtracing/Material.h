#pragma once
#include <Vec3.h>
#include <optional>

#include "Texture.h"

class Material {
private:
	Vec3 diffuse;
	Vec3 specular;

	float local;
	std::optional<float> IOR;
	std::optional<Texture> texture;
	Vec3 emission;

public:
	Material(const Vec3& diffuse)
		: Material(diffuse, Vec3{}, 1.0f)
	{ }

	Material(const Vec3& diffuse, const Vec3& specular, float local)
		: Material(diffuse, specular, local, std::nullopt)
	{ }
	
	/// <param name="diffuse">diffuse object color</param>
	/// <param name="specular">specular object color</param>
	/// <param name="local">local reflectivity, percentage of local illumination, 1 - local is the percentage of light
	///						that gets reflected or reflected and refracted according to Schlick's approximation of the
	///						Fresnel equations</param>
	/// <param name="IOR">Index Of Refraction</param>
	Material(const Vec3& diffuse, const Vec3& specular, float local, const std::optional<float>& IOR)
		: Material(diffuse, specular, local, IOR, std::nullopt)
	{ }

	Material(const Vec3& diffuse, const Vec3& specular, float local, const std::optional<float>& IOR, const std::optional<Texture>& texture, const Vec3& emission = Vec3{})
		: diffuse(diffuse), specular(specular), local(local), IOR(IOR), texture(texture), emission(emission)
	{ }
	
	Vec3 getDiffuse() const;

	Vec3 getSpecular() const;

	bool reflects() const;

	bool refracts() const;

	bool hasTexture() const;

	bool emits() const;

	Vec3 getEmission() const;

	std::optional<float> getIndexOfRefraction() const;

	float getReflectivity(float cosI) const;

	std::optional<Texture> getTexture() const;
};

