#pragma once
#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include <climits>

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

template <typename T>
Vec8f gather(const T* in, const Vec8i index) {
	auto mask = Vec8i(UINT_MAX >> 8 * (4 - sizeof(T)));
	return to_float(mask & _mm256_i32gather_epi32(in, index, sizeof(*in)));
}

template <>
Vec8f gather(const float* in, const Vec8i index) {
	return _mm256_i32gather_ps(in, index, sizeof(*in));
}

template <typename T>
void store2out(const int* in, T* out, int length);

template <>
void store2out(const int* in, uint8_t* out, int length) {
	int i = 0;
	for (; i < length - 31; i += 32) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		Vec8i c = Vec8i().load(in + i + 16);
		Vec8i d = Vec8i().load(in + i + 24);
		// usar packus aqui pq satura se os nrs forem maiores k 255 ou menores k
		// 0, portanto temos o clamp de graça
		Vec32uc(_mm256_packus_epi16(compress_saturated(a, c), compress_saturated(b, d))).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i] = in[i];

	return;
}

__attribute__((noinline)) Vec8i vecModCond(const Vec8i vec, const Vec8ib mask, const int div) {
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

