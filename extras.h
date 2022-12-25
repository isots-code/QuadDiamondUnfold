#pragma once
#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

template <typename T>
Vec8f gather(const T* in, const Vec8i index) {
	if constexpr (std::same_as<T, float>)
		return _mm256_i32gather_ps(in, index, sizeof(*in));
	else {
		auto mask = Vec8i(UINT_MAX >> 8 * (4 - sizeof(T)));
		return to_float(mask & _mm256_i32gather_epi32(in, index, sizeof(*in)));
	}
}
