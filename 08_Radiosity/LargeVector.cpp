#include "LargeVector.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

LargeVector::LargeVector(size_t size, float value)
	: values(size, value)
{
}

size_t LargeVector::size() const {
	return values.size();
}

void LargeVector::resize(size_t size, float value) {
	values.assign(size, value);
}

float& LargeVector::operator[](size_t index) {
	return values[index];
}

float LargeVector::operator[](size_t index) const {
	return values[index];
}

float LargeVector::maxAbsDifference(const LargeVector& other) const {
	if (values.size() != other.values.size())
		throw std::runtime_error("LargeVector size mismatch");

	float result = 0.0f;
	for (size_t i = 0; i < values.size(); ++i)
		result = std::max(result, std::fabs(values[i] - other.values[i]));

	return result;
}
