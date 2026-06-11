#include "RadiositySolver.h"

#include "Intersection.h"
#include "Ray.h"
#include "Scene.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

constexpr float pi = 3.14159265358979323846f;

float channelValue(const Vec3& value, int channel) {
	return value.e[size_t(channel)];
}

float formFactorEstimate(const RadiosityPatch& receiver, const RadiosityPatch& emitter) {
	(void)receiver;
	(void)emitter;
	(void)pi;

	// TODO: Implement the center-to-center form-factor approximation.
	//
	// F_ij ~= A_j cos(theta_i) cos(theta_j) / (pi r^2)
	//
	// receiver is patch i, emitter is patch j.
	// 1. Compute the vector from receiver.center to emitter.center.
	// 2. Compute r^2 from that vector and return 0 for zero distance.
	// 3. Normalize the vector to get the direction from receiver to emitter.
	// 4. Compute cos(theta_i) with receiver.normal.
	// 5. Compute cos(theta_j) with emitter.normal and the opposite direction.
	// 6. Return 0 if either cosine is negative or zero.
	// 7. Return emitter.area * cos(theta_i) * cos(theta_j) / (pi * r^2).
	return 0.0f;
}

} // namespace

RadiositySolver::RadiositySolver(size_t iterationCount)
	: iterationCount(iterationCount)
{
}

void RadiositySolver::setIterationCount(size_t iterationCount) {
	this->iterationCount = iterationCount;
}

void RadiositySolver::solve(const Scene& scene, std::vector<RadiosityPatch>& patches) {
	prepareFormFactorComputation(patches);
	while (!computeFormFactors(scene, patches, std::numeric_limits<size_t>::max())) {
	}
	finishSolve(patches);
}

void RadiositySolver::prepareFormFactorComputation(const std::vector<RadiosityPatch>& patches) {
	const size_t patchCount = patches.size();
	formFactors.resize(patchCount, patchCount, 0.0f);
	currentReceiver = 0;
	currentEmitter = 0;
	currentRowSum = 0.0f;
	formFactorComputationPrepared = true;
	formFactorComputationFinished = patchCount == 0;
}

bool RadiositySolver::computeFormFactors(const Scene& scene,
                                         const std::vector<RadiosityPatch>& patches,
                                         size_t maxFormFactors) {
	if (!formFactorComputationPrepared)
		prepareFormFactorComputation(patches);

	if (formFactorComputationFinished)
		return true;

	const size_t patchCount = patches.size();
	size_t computedFormFactors = 0;
	maxFormFactors = std::max<size_t>(1, maxFormFactors);

	// The computation is written as an incremental double loop. Each call
	// continues where the previous call stopped, so the application can draw a
	// progress frame between batches of form-factor entries.
	while (currentReceiver < patchCount && computedFormFactors < maxFormFactors) {
		if (currentEmitter < patchCount) {
			// A patch does not exchange energy with itself in this simple model.
			// We also require visibility; otherwise walls or objects between
			// the two patch centers should block the transfer.
			if (currentReceiver != currentEmitter && visible(scene, patches[currentReceiver], patches[currentEmitter])) {
				const float formFactor = formFactorEstimate(patches[currentReceiver], patches[currentEmitter]);
				formFactors(currentReceiver, currentEmitter) = formFactor;
				currentRowSum += formFactor;
			}

			++currentEmitter;
			++computedFormFactors;
			continue;
		}

		// The center-to-center approximation can overestimate a row, especially
		// for coarse patches. Clamping each row below one keeps the transport
		// matrix energy stable for this exercise implementation.
		if (currentRowSum > 0.95f) {
			const float scale = 0.95f / currentRowSum;
			for (size_t emitter = 0; emitter < patchCount; ++emitter)
				formFactors(currentReceiver, emitter) *= scale;
		}

		++currentReceiver;
		currentEmitter = 0;
		currentRowSum = 0.0f;
	}

	formFactorComputationFinished = currentReceiver >= patchCount;
	return formFactorComputationFinished;
}

void RadiositySolver::finishSolve(std::vector<RadiosityPatch>& patches) const {
	if (!formFactorComputationFinished)
		throw std::runtime_error("Cannot finish radiosity solve before form factors are complete.");

	for (int channel = 0; channel < 3; ++channel) {
		const LargeVector radiosity = solveChannel(formFactors, patches, channel);
		for (size_t i = 0; i < patches.size(); ++i)
			patches[i].radiosity.e[size_t(channel)] = radiosity[i];
	}
}

float RadiositySolver::formFactorProgress() const {
	const size_t rowCount = formFactors.rows();
	const size_t columnCount = formFactors.columns();
	if (rowCount == 0 || columnCount == 0)
		return formFactorComputationFinished ? 1.0f : 0.0f;

	const size_t finishedEntries = currentReceiver * columnCount + currentEmitter;
	const size_t totalEntries = rowCount * columnCount;
	return std::min(1.0f, float(finishedEntries) / float(totalEntries));
}

size_t RadiositySolver::completedFormFactorRows() const {
	return currentReceiver;
}

size_t RadiositySolver::totalFormFactorRows() const {
	return formFactors.rows();
}

LargeVector RadiositySolver::solveChannel(const LargeMatrix& formFactors,
                                          const std::vector<RadiosityPatch>& patches,
                                          int channel) const {
	const size_t patchCount = patches.size();
	LargeMatrix transport(patchCount, patchCount, 0.0f);
	LargeVector emission(patchCount, 0.0f);
	LargeVector current(patchCount, 0.0f);

	for (size_t receiver = 0; receiver < patchCount; ++receiver) {
		const float reflectance = channelValue(patches[receiver].reflectance, channel);
		emission[receiver] = channelValue(patches[receiver].emission, channel);
		current[receiver] = emission[receiver];
		for (size_t emitter = 0; emitter < patchCount; ++emitter)
			transport(receiver, emitter) = reflectance * formFactors(receiver, emitter);
	}

	for (size_t iteration = 0; iteration < iterationCount; ++iteration) {
		LargeVector next = transport * current;
		for (size_t i = 0; i < patchCount; ++i)
			next[i] += emission[i];

		if (next.maxAbsDifference(current) < 0.0001f)
			return next;

		current = next;
	}

	return current;
}

bool RadiositySolver::visible(const Scene& scene,
                              const RadiosityPatch& from,
                              const RadiosityPatch& to) const {
	(void)scene;
	(void)from;
	(void)to;
	(void)visibilityEpsilon;

	// TODO: Test whether two patch centers can see each other.
	//
	// 1. Compute the segment from from.center to to.center.
	// 2. Compute its length and return false if it is too small.
	// 3. Normalize the segment to get the ray direction.
	// 4. Start the ray at from.center + direction * visibilityEpsilon.
	// 5. Intersect the ray with the scene.
	// 6. The patches are visible if there is no hit before the target center.
	return false;
}
