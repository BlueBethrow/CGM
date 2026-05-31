#include "LargeMatrix.h"

#include <stdexcept>

LargeMatrix::LargeMatrix(size_t rows, size_t columns, float value) {
	resize(rows, columns, value);
}

void LargeMatrix::resize(size_t rows, size_t columns, float value) {
	rowCount = rows;
	columnCount = columns;
	values.assign(rows * columns, value);
}

size_t LargeMatrix::rows() const {
	return rowCount;
}

size_t LargeMatrix::columns() const {
	return columnCount;
}

float& LargeMatrix::operator()(size_t row, size_t column) {
	return values[row * columnCount + column];
}

float LargeMatrix::operator()(size_t row, size_t column) const {
	return values[row * columnCount + column];
}

LargeVector LargeMatrix::operator*(const LargeVector& vector) const {
	if (columnCount != vector.size())
		throw std::runtime_error("LargeMatrix / LargeVector size mismatch");

	LargeVector result(rowCount, 0.0f);
	for (size_t row = 0; row < rowCount; ++row) {
		float sum = 0.0f;
		for (size_t column = 0; column < columnCount; ++column)
			sum += (*this)(row, column) * vector[column];
		result[row] = sum;
	}

	return result;
}
