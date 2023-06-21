

#include "FixedPoint.h"

FixedPoint::FixedPoint(int32_t val, int frac_bits) : value(val), fractional_bits(frac_bits) {}

FixedPoint FixedPoint::operator+(const FixedPoint& other) const {
    int32_t sum = value + other.value;
    return FixedPoint(sum, fractional_bits);
}

FixedPoint FixedPoint::operator-(const FixedPoint& other) const {
    int32_t diff = value - other.value;
    return FixedPoint(diff, fractional_bits);
}

FixedPoint FixedPoint::operator*(const FixedPoint& other) const {
    int32_t product = (static_cast<int64_t>(value) * other.value) >> fractional_bits;
    return FixedPoint(product, fractional_bits);
}

FixedPoint FixedPoint::operator/(const FixedPoint& other) const {
    int32_t div = (static_cast<int64_t>(value) << fractional_bits) / other.value;
    return FixedPoint(div, fractional_bits);
}

float FixedPoint::toFloat() const {
    return (float(value) / (1 << fractional_bits));
}

FixedPoint FixedPoint::fromFloat(float fp32, int frac_bits) {
    int32_t fixed_value = static_cast<int32_t>(fp32 * (1 << frac_bits));
    return FixedPoint(fixed_value, frac_bits);
}
