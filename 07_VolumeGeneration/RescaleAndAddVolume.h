#pragma once

#include <array>
#include <cstdint>

#include "Volume.h"

struct RescaleAndAddVolumeParameters {
  RescaleAndAddVolumeParameters() = default;

  RescaleAndAddVolumeParameters(const size_t width,
                                const size_t height,
                                const size_t depth,
                                const Vec3& scale,
                                const uint32_t seed,
                                const size_t octaves,
                                const float baseFrequency,
                                const float lacunarity,
                                const float roughness,
                                const bool billowyNoise,
                                const float coverage,
                                const float densityScale,
                                const float densityOffset,
                                const float densityExponent,
                                const float radius,
                                const float envelopeSoftness,
                                const float silhouetteNoiseFrequency,
                                const float silhouetteNoiseStrength,
                                const float detailErosion,
                                const bool computeNormals) :
    width{width},
    height{height},
    depth{depth},
    scale{scale},
    seed{seed},
    octaves{octaves},
    baseFrequency{baseFrequency},
    lacunarity{lacunarity},
    roughness{roughness},
    billowyNoise{billowyNoise},
    coverage{coverage},
    densityScale{densityScale},
    densityOffset{densityOffset},
    densityExponent{densityExponent},
    radius{radius},
    envelopeSoftness{envelopeSoftness},
    silhouetteNoiseFrequency{silhouetteNoiseFrequency},
    silhouetteNoiseStrength{silhouetteNoiseStrength},
    detailErosion{detailErosion},
    computeNormals{computeNormals}
  {}

  size_t width{192};
  size_t height{64};
  size_t depth{128};
  Vec3 scale{1.0f, 1.0f, 1.0f};

  uint32_t seed{1337};
  size_t octaves{5};
  float baseFrequency{2.1f};
  float lacunarity{2.0f};
  float roughness{0.54f};
  bool billowyNoise{true};

  float coverage{0.38f};
  float densityScale{0.75f};
  float densityOffset{0.0f};
  float densityExponent{1.15f};
  float radius{0.56f};
  float envelopeSoftness{0.12f};
  float silhouetteNoiseFrequency{2.4f};
  float silhouetteNoiseStrength{0.28f};
  float detailErosion{0.22f};
  bool computeNormals{false};
};

class RescaleAndAddVolume {
public:
  RescaleAndAddVolume(const RescaleAndAddVolumeParameters& parameters = {});
  Volume generate();

  Volume volume;

private:
  void generate(const RescaleAndAddVolumeParameters& parameters);
  void buildPermutation(const uint32_t seed);

  float fractalNoise(const float x, const float y, const float z) const;
  float perlin(const float x, const float y, const float z) const;
  float gradient(const int hash, const float x, const float y, const float z) const;
  float cloudEnvelope(const float x, const float y, const float z) const;
  int perm(const int index) const;

  static float fade(const float t);
  static float lerp(const float a, const float b, const float t);
  static float smoothStep(const float edge0, const float edge1, const float x);

  RescaleAndAddVolumeParameters parameters;
  std::array<int, 512> permutation;
};
