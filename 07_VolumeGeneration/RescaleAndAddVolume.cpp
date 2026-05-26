#include "RescaleAndAddVolume.h"

#include <algorithm>
#include <cmath>

RescaleAndAddVolume::RescaleAndAddVolume(const RescaleAndAddVolumeParameters& parameters) :
  parameters{parameters}
{
}

Volume RescaleAndAddVolume::generate() {
  generate(parameters);
  return volume;
}

void RescaleAndAddVolume::generate(const RescaleAndAddVolumeParameters& parameters) {
  this->parameters = parameters;
  buildPermutation(parameters.seed);

  volume.width = std::max<size_t>(1, parameters.width);
  volume.height = std::max<size_t>(1, parameters.height);
  volume.depth = std::max<size_t>(1, parameters.depth);
  volume.scale = parameters.scale;
  volume.normalizeScale();

  volume.data.resize(volume.width * volume.height * volume.depth);

  std::fill(volume.data.begin(), volume.data.end(), uint8_t{0});

  // TODO: Fill volume.data with cloud densities:
  // 1. Iterate over all voxels and map u/v/w to normalized coordinates x/y/z in [0, 1].
  // 2. Evaluate fractalNoise(x, y, z) for the rescale-and-add noise value.
  // 3. Evaluate cloudEnvelope(x, y, z) to fade the cloud near its boundary.
  // 4. Add high-frequency detail with perlin(...) and parameters.detailErosion.
  // 5. Use parameters.coverage, densityExponent, densityScale, and densityOffset
  //    to shape the final density.
  // 6. Store the clamped density as an 8-bit value in volume.data.

  if (parameters.computeNormals) {
    volume.computeNormals();
  } else {
    volume.normals.clear();
  }
}

void RescaleAndAddVolume::buildPermutation(const uint32_t seed) {
  std::array<int, 256> values;
  for (size_t i = 0; i < values.size(); ++i) {
    values[i] = int(i);
  }

  uint32_t state = seed == 0 ? 1 : seed;
  for (size_t i = values.size() - 1; i > 0; --i) {
    state = state * 1664525u + 1013904223u;
    const size_t j = size_t(state % uint32_t(i + 1));
    std::swap(values[i], values[j]);
  }

  for (size_t i = 0; i < permutation.size(); ++i) {
    permutation[i] = values[i & 255];
  }
}

float RescaleAndAddVolume::fractalNoise(const float x, const float y, const float z) const {
  (void)x;
  (void)y;
  (void)z;

  // TODO: Implement rescale-and-add noise:
  // - Start with frequency = parameters.baseFrequency and amplitude = 1.
  // - For parameters.octaves iterations, sample perlin(x * frequency, ...).
  // - If parameters.billowyNoise is true, convert the octave to billowy noise
  //   with 1 - abs(octaveValue); otherwise remap Perlin from [-1, 1] to [0, 1].
  // - Accumulate octaveValue * amplitude and normalize by the sum of amplitudes.
  // - Multiply amplitude by parameters.roughness and frequency by parameters.lacunarity.
  return 0.0f;
}

float RescaleAndAddVolume::perlin(const float inputX, const float inputY, const float inputZ) const {
  float x = inputX;
  float y = inputY;
  float z = inputZ;

  const int cellX = int(std::floor(x)) & 255;
  const int cellY = int(std::floor(y)) & 255;
  const int cellZ = int(std::floor(z)) & 255;

  x -= std::floor(x);
  y -= std::floor(y);
  z -= std::floor(z);

  const float u = fade(x);
  const float v = fade(y);
  const float w = fade(z);

  const int a = perm(cellX) + cellY;
  const int aa = perm(a) + cellZ;
  const int ab = perm(a + 1) + cellZ;
  const int b = perm(cellX + 1) + cellY;
  const int ba = perm(b) + cellZ;
  const int bb = perm(b + 1) + cellZ;

  return lerp(
    lerp(
      lerp(gradient(perm(aa), x, y, z),
           gradient(perm(ba), x - 1.0f, y, z),
           u),
      lerp(gradient(perm(ab), x, y - 1.0f, z),
           gradient(perm(bb), x - 1.0f, y - 1.0f, z),
           u),
      v),
    lerp(
      lerp(gradient(perm(aa + 1), x, y, z - 1.0f),
           gradient(perm(ba + 1), x - 1.0f, y, z - 1.0f),
           u),
      lerp(gradient(perm(ab + 1), x, y - 1.0f, z - 1.0f),
           gradient(perm(bb + 1), x - 1.0f, y - 1.0f, z - 1.0f),
           u),
      v),
    w);
}

float RescaleAndAddVolume::gradient(const int hash, const float x, const float y, const float z) const {
  const int h = hash & 15;
  const float u = h < 8 ? x : y;
  const float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float RescaleAndAddVolume::cloudEnvelope(const float x, const float y, const float z) const {
  (void)x;
  (void)y;
  (void)z;

  // TODO: Implement the cloud envelope:
  // - Compute a noisy radius with perlin(...) and parameters.silhouetteNoiseFrequency.
  // - Measure the distance from the center of the volume.
  // - Use parameters.radius, silhouetteNoiseStrength, and envelopeSoftness to
  //   return a smooth spherical falloff in [0, 1].
  return 1.0f;
}

int RescaleAndAddVolume::perm(const int index) const {
  return permutation[size_t(index & 511)];
}

float RescaleAndAddVolume::fade(const float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float RescaleAndAddVolume::lerp(const float a, const float b, const float t) {
  return a + t * (b - a);
}

float RescaleAndAddVolume::smoothStep(const float edge0, const float edge1, const float x) {
  const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}
