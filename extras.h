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

template <typename T, typename U, int NT>
void store2out(const T* in, U* out, int length);

template <int NT>
void store2out(const float* in, uint8_t* out, int length) {
	int i = 0;
	for (; i < length - 31; i += 32) {
		Vec8i a = truncatei(Vec8f().load(in + i) + 0.5f);
		Vec8i b = truncatei(Vec8f().load(in + i + 8) + 0.5f);
		Vec8i c = truncatei(Vec8f().load(in + i + 16) + 0.5f);
		Vec8i d = truncatei(Vec8f().load(in + i + 24) + 0.5f);
		// usar packus aqui pq satura se os nrs forem maiores k 255 ou menores k
		// 0, portanto temos o clamp de graça
		auto temp = Vec32uc(_mm256_packus_epi16(compress_saturated(a, c), compress_saturated(b, d)));
		if constexpr (NT == false)
			temp.store(out + i);
		else
			temp.store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i] = in[i] + 0.5f;

	return;
}

template <int NT>
void store2out(const int* in, uint8_t* out, int length) {
	int i = 0;
	for (; i < length - 31; i += 32) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		Vec8i c = Vec8i().load(in + i + 16);
		Vec8i d = Vec8i().load(in + i + 24);
		// usar packus aqui pq satura se os nrs forem maiores k 255 ou menores k
		// 0, portanto temos o clamp de graça
		auto temp = Vec32uc(_mm256_packus_epi16(compress_saturated(a, c), compress_saturated(b, d)));
		if constexpr (NT == false)
			temp.store(out + i);
		else
			temp.store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i] = in[i];

	return;
}

__attribute__((noinline)) Vec8i vecModCond(const Vec8i vec, const Vec8ib mask, const int div) {
	int test[vec.size()];
	vec.store(test);
	volatile int mask_[mask.size()];
	mask.store((int*)mask_);

#pragma unroll(1)
	for (auto i = 0; i < vec.size(); i++)
		test[i] = mask_[i] ? test[i] % div : test[i];

	Vec8i ret = Vec8i().load(test);
	ret = if_add(ret < 0, ret, div);
	return ret;
}

