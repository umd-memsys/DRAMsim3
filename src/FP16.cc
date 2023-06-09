#include "FP16.h"

FP16::FP16(fp16 val) : value(val) {}

FP16 FP16::operator+(const FP16& other) const {
    fp16 sum = value + other.value;
    return FP16(sum);
}

FP16 FP16::operator-(const FP16& other) const {
    fp16 diff = value - other.value;
    return FP16(diff);
}

FP16 FP16::operator*(const FP16& other) const {
    fp16 product = value * other.value;
    return FP16(product);
}

FP16 FP16::operator/(const FP16& other) const {
    fp16 div = value / other.value;
    return FP16(div);
}

float FP16::toFloat() const {
    float fp32 = value;
    return fp32; 
}

FP16 FP16::fromFloat(float fp32) {
    fp16 val0(fp32);
    return val0;
}