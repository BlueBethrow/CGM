#pragma once
#include <cmath>
#include <cstddef>
#include <Vec3.h>
#include <Image.h>
#include <unordered_map>
#include <vector>

#include "Camera.h"
#include "Photon.h"
#include "Scene.h"


struct RaySetup
{
public:
	Vec3 bottomLeft;
	Vec3 rayOrigin;
	Vec3 dX;
	Vec3 dY;
};

class PhotonMapper
{
public:
	struct PhotonGrid {
		std::unordered_map<long long, std::vector<size_t>> cells;
	};

private:
	int recDepth;
	int numSamplesX;
	int numSamplesY;
	Camera camera;
	Scene scene;
	std::vector<Photon> photons;
	PhotonGrid photonGrid;
	size_t targetPhotonCount{0};
	int photonTraceDepth{5};

public:
	PhotonMapper(int recDepth, int numSamples)
		: recDepth(recDepth)
	{
		numSamplesX = (int)sqrtf(float(numSamples));
		numSamplesY = numSamples / numSamplesX;
	}

	void setCamera(const Camera& camera);
	void setScene(const Scene& scene);
	void render(Image& img);
	void preparePhotonEmission(size_t photonCount);
	bool emitPhotons(size_t photonPathCount);
	void setPhotonMap(const std::vector<Photon>& photons);
	void setPhotonMap(const std::vector<Photon>& photons, const PhotonGrid& photonGrid);
	const std::vector<Photon>& getPhotons() const;
	const PhotonGrid& getPhotonGrid() const;
	size_t getPhotonCount() const;
	size_t getTargetPhotonCount() const;
	std::vector<float> getPhotonPointData() const;

private:
	Vec3 traceRay(const Ray& r) const;
	Vec3 traceLocalRay(const Ray& localRay, int depth) const;
	Ray computeRay(float x, float y, const RaySetup& rs) const;
	RaySetup computeRaySetup(const Image& img) const;
	void tracePhotonPath(const Vec3& origin, const Vec3& direction, const Vec3& power);
	void storePhoton(const Photon& photon);
	void initializePhotonGrid();
	void insertPhotonInGrid(const Vec3& position, size_t photonIndex);
	Vec3 estimatePhotonRadiance(const Ray& localRay, const Intersection& intersection) const;
	void buildPhotonGrid();
	long long photonGridKey(int x, int y, int z) const;
	long long photonGridKey(const Vec3& position) const;
	Vec3 sampleDiffuseDirection(const Vec3& normal) const;
	Vec3 getDiffuseColor(const Intersection& intersection) const;
};

