#pragma once
#include <Image.h>
#include <Vec3.h>

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "Scene.h"

struct RaySetup
{
public:
	Vec3 bottomLeft;
	Vec3 rayOrigin;
	Vec3 dX;
	Vec3 dY;
};

class Pathtracer
{
private:
	int maxDepth;
	uint32_t samplesPerRender;
	uint32_t maxSamples;
	uint32_t accumulatedSamples{0};
	std::vector<Vec3> accumulation;
	Camera camera;
	Scene scene;

public:
	Pathtracer(int maxDepth, uint32_t samplesPerRender, uint32_t maxSamples)
		: maxDepth(maxDepth), samplesPerRender(samplesPerRender), maxSamples(maxSamples)
	{ }

	void setCamera(const Camera& camera);
	void setScene(const Scene& scene);
	void reset();
	void render(Image& img);
	uint32_t getSampleCount() const;
	uint32_t getMaxSampleCount() const;
	void setMaxSampleCount(uint32_t maxSamples);
	bool isFinished() const;

private:
	Vec3 tracePath(const Ray& ray) const;
	Ray computeRay(float x, float y, const RaySetup& rs) const;
	RaySetup computeRaySetup(const Image& img) const;
};
