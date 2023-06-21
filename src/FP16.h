#ifndef FP16_H
#define FP16_H

#include "half.hpp"

typedef half_float::half fp16;

namespace dramsim3 {


struct FP16 {
    fp16 value;

    FP16(fp16 val);

    FP16 operator+(const FP16& other) const;
    FP16 operator-(const FP16& other) const;
    FP16 operator*(const FP16& other) const;
    FP16 operator/(const FP16& other) const;

};

}


#endif  // FP16_H

