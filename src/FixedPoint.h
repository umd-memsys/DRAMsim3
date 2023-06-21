#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <cstdint>

struct FixedPoint {
    int32_t value;
    int fractional_bits;

    FixedPoint(int32_t val, int frac_bits);

    FixedPoint operator+(const FixedPoint& other) const;
    FixedPoint operator-(const FixedPoint& other) const;
    FixedPoint operator*(const FixedPoint& other) const;
    FixedPoint operator/(const FixedPoint& other) const;

    float toFloat() const;

    static FixedPoint fromFloat(float fp32, int frac_bits);
    
};

#endif