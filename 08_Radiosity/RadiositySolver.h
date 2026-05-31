#pragma once

#include "LargeMatrix.h"
#include "RadiosityPatch.h"

#include <cstddef>
#include <vector>

class Scene;

class RadiositySolver {
private:
	size_t iterationCount{120};
	float visibilityEpsilon{0.0005f};
	LargeMatrix formFactors;
	size_t currentReceiver{0};
	size_t currentEmitter{0};
	float currentRowSum{0.0f};
	bool formFactorComputationPrepared{false};
	bool formFactorComputationFinished{false};

public:
	explicit RadiositySolver(size_t iterationCount = 120);

	void setIterationCount(size_t iterationCount);
	void solve(const Scene& scene, std::vector<RadiosityPatch>& patches);
	void prepareFormFactorComputation(const std::vector<RadiosityPatch>& patches);
	bool computeFormFactors(const Scene& scene,
	                        const std::vector<RadiosityPatch>& patches,
	                        size_t maxFormFactors);
	void finishSolve(std::vector<RadiosityPatch>& patches) const;
	float formFactorProgress() const;
	size_t completedFormFactorRows() const;
	size_t totalFormFactorRows() const;

private:
	LargeVector solveChannel(const LargeMatrix& formFactors,
	                         const std::vector<RadiosityPatch>& patches,
	                         int channel) const;
	bool visible(const Scene& scene,
	             const RadiosityPatch& from,
	             const RadiosityPatch& to) const;
};
