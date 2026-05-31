#pragma once

#include "IntersectableObject.h"
#include "RadiosityPatch.h"

#include <Mat4.h>
#include <Vec3.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

class Scene {
private:
	std::vector<std::shared_ptr<const IntersectableObject>> sceneObjects;
	Vec3 backgroundColor;
	Mat4 model;

public:
	Scene()
		: Scene(Vec3{0.02f, 0.02f, 0.02f})
	{
	}

	explicit Scene(const Vec3& backgroundColor)
		: backgroundColor(backgroundColor)
	{
	}

	void addObject(std::shared_ptr<const IntersectableObject> object);
	void setModel(const Mat4& model);
	Mat4 getModel() const;
	Vec3 getBackgroundcolor() const;

	std::optional<Intersection> intersect(const Ray& ray) const;
	std::vector<RadiosityPatch> createPatches() const;
	std::vector<float> getTriangleData(const std::vector<RadiosityPatch>& patches) const;
	std::vector<float> getSmoothedTriangleData(const std::vector<RadiosityPatch>& patches) const;
	std::vector<float> getFormFactorProgressTriangleData(const std::vector<RadiosityPatch>& patches,
	                                                     size_t completedRows) const;

	static Scene genCornellBox(size_t wallTessellationFactor = 4, size_t sphereTessellationFactor = 4);
};
