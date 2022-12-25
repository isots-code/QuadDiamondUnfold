#pragma once
#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

template <typename T>
Vec8f gather(const T* in, const Vec8i index) {
	auto mask = Vec8i(UINT_MAX >> 8 * (4 - sizeof(T)));
	return to_float(mask & _mm256_i32gather_epi32(in, index, sizeof(*in)));
}

//__attribute__((noinline)) 
Vec8i vecModCond(const Vec8i vec, const Vec8ib mask, const int div) {
	int test[Vec8i::size()];
	vec.store(test);
	volatile int mask_[Vec8ib::size()];
	mask.store((int*)mask_);

#pragma unroll(1)
	for (auto i = 0; i < Vec8i::size(); i++)
		test[i] = mask_[i] ? test[i] % div : test[i];

	Vec8i ret = Vec8i().load(test);
	ret = if_add(ret < 0, ret, div);
	return ret;
}

