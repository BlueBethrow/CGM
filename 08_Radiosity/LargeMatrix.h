#pragma once

#include "LargeVector.h"

#include <cstddef>
#include <vector>

class LargeMatrix {
private:
	size_t rowCount{0};
	size_t columnCount{0};
	std::vector<float> values;

public:
	LargeMatrix() = default;
	LargeMatrix(size_t rows, size_t columns, float value = 0.0f);

	void resize(size_t rows, size_t columns, float value = 0.0f);
	size_t rows() const;
	size_t columns() const;

	float& operator()(size_t row, size_t column);
	float operator()(size_t row, size_t column) const;

	LargeVector operator*(const LargeVector& vector) const;
};
