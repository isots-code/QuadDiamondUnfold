#pragma once
#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include <climits>
#include <algorithm>

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

typedef struct { uint8_t i; } __attribute__((__packed__)) bits8_t;
typedef struct { uint16_t i; } __attribute__((__packed__)) bits10_t;
typedef struct { uint16_t i; } __attribute__((__packed__)) bits12_t;
typedef struct { uint16_t i; } __attribute__((__packed__)) bits14_t;
typedef struct { uint16_t i; } __attribute__((__packed__)) bits16_t;

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
void store2out(const int* in, bits8_t* out, int length) {
	int i = 0;
	for (; i < length - Vec32uc::size() - 1; i += Vec32uc::size()) {
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
		out[i].i = std::max(std::min(in[i], 255), 0);

	return;
}

template <>
void store2out(const int* in, bits10_t* out, int length) {
	int i = 0;
	for (; i < length - Vec16us::size() - 1; i += Vec16us::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		max(min(compress_saturated(a, b), Vec16s(1024)), Vec16s(0)).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i].i = std::max(std::min(in[i], 1024), 0);

	return;
}

template <>
void store2out(const int* in, bits12_t* out, int length) {
	int i = 0;
	for (; i < length - Vec16us::size() - 1; i += Vec16us::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		max(min(compress_saturated(a, b), Vec16s(4096)), Vec16s(0)).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i].i = std::max(std::min(in[i], 4096), 0);

	return;
}

template <>
void store2out(const int* in, bits14_t* out, int length) {
	int i = 0;
	for (; i < length - Vec16us::size() - 1; i += Vec16us::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		max(min(compress_saturated(a, b), Vec16s(16384)), Vec16s(0)).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i].i = std::max(std::min(in[i], 16384), 0);

	return;
}

template <>
void store2out(const int* in, bits16_t* out, int length) {
	int i = 0;
	for (; i < length - Vec16us::size() - 1; i += Vec16us::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		compress_saturated(a, b).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i].i = std::max(std::min(in[i], 65536), 0);

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

