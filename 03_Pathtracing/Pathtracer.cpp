#define _USE_MATH_DEFINES
#include "Pathtracer.h"

#include <algorithm>
#include <cmath>

namespace {

float linearToSRGB(float value) {
	const float clamped = std::clamp(value, 0.0f, 1.0f);
	return std::pow(clamped, 1.0f / 2.2f);
}

Vec3 linearToSRGB(const Vec3& color) {
	return Vec3{linearToSRGB(color.r), linearToSRGB(color.g), linearToSRGB(color.b)};
}

} // namespace

void Pathtracer::setCamera(const Camera& camera)
{
	this->camera = camera;
	reset();
}

void Pathtracer::setScene(const Scene& scene)
{
	this->scene = scene;
	reset();
}

void Pathtracer::reset()
{
	accumulatedSamples = 0;
	accumulation.clear();
}

uint32_t Pathtracer::getSampleCount() const
{
	return accumulatedSamples;
}

uint32_t Pathtracer::getMaxSampleCount() const
{
	return maxSamples;
}

void Pathtracer::setMaxSampleCount(uint32_t maxSamples)
{
	this->maxSamples = maxSamples;
}

bool Pathtracer::isFinished() const
{
	return accumulatedSamples >= maxSamples;
}

void Pathtracer::render(Image& img)
{
	if (isFinished())
		return;

	const size_t pixelCount = size_t(img.width) * size_t(img.height);
	if (accumulation.size() != pixelCount)
	{
		accumulation.assign(pixelCount, Vec3{});
		accumulatedSamples = 0;
	}

	const RaySetup rs = computeRaySetup(img);
	const uint32_t samplesThisPass = std::min(samplesPerRender, maxSamples - accumulatedSamples);

	for (uint32_t y = 0; y < img.height; ++y)
	{
		for (uint32_t x = 0; x < img.width; ++x)
		{
			Vec3 color;
			for (uint32_t sample = 0; sample < samplesThisPass; ++sample)
			{
				const Ray ray = computeRay(float(x) + staticRand.rand01(),
				                          float(y) + staticRand.rand01(),
				                          rs);
				color = color + tracePath(ray);
			}

			const size_t index = size_t(y) * size_t(img.width) + size_t(x);
			accumulation[index] = accumulation[index] + color;
			const Vec3 average = accumulation[index] / float(accumulatedSamples + samplesThisPass);
			const Vec3 displayColor = linearToSRGB(average);

			img.setNormalizedValue(x, y, 0, displayColor.r);
			img.setNormalizedValue(x, y, 1, displayColor.g);
			img.setNormalizedValue(x, y, 2, displayColor.b);
			img.setValue(x, y, 3, 255);
		}
	}

	accumulatedSamples += samplesThisPass;
}

Vec3 Pathtracer::tracePath(const Ray& ray) const
{
	return scene.tracePath(ray, maxDepth);
}

Ray Pathtracer::computeRay(float x, float y, const RaySetup& rs) const
{
	Vec3 dir{ rs.bottomLeft + rs.dX * x + rs.dY * y };
	dir = Vec3::normalize(dir);
	return Ray{ rs.rayOrigin, dir };
}

RaySetup Pathtracer::computeRaySetup(const Image& img) const
{
	RaySetup rs;

	const Vec3 forwardDir = camera.getViewDir();
	const Vec3 upDir = camera.getUpDir();
	const float openingAngle = float(camera.getFoV() * M_PI/180.0);
	rs.rayOrigin = camera.getEyePoint();

	const float aspectRatio = float(img.width) / float(img.height);

	const Vec3 rightDir = Vec3::cross(forwardDir, upDir);

	const Vec3 rowVector = rightDir * (tan(openingAngle / 2.0f) * aspectRatio);
	const Vec3 columnVector = upDir * tan(openingAngle / 2.0f);

	rs.dX = rowVector * 2.0f / float(img.width);
	rs.dY = columnVector * 2.0f / float(img.height);

	rs.bottomLeft = forwardDir - columnVector - rowVector;

	return rs;
}
