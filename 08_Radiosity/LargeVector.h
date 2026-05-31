#pragma once

#include <cstddef>
#include <vector>

class LargeVector {
private:
	std::vector<float> values;

public:
	LargeVector() = default;
	explicit LargeVector(size_t size, float value = 0.0f);

	size_t size() const;
	void resize(size_t size, float value = 0.0f);

	float& operator[](size_t index);
	float operator[](size_t index) const;

	float maxAbsDifference(const LargeVector& other) const;
};
