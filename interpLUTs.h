#pragma once

#include "frameData.h"
//#include "frameDataCustom.h"

#include <cmath>
#include <numbers>

struct nearest : public frameData {
	nearest(int dim, bitPerSubPixel_t bits) : frameData(dim, 1, bits) { buildFrameCoeffs(); };
	nearest(int dim, bitPerSubPixel_t bits, int numThreads) : frameData(dim, 1, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		(void)x;
		return std::vector<float>({ 1.0f });
	}
};

struct linear : public frameData {
	linear(int dim, bitPerSubPixel_t bits) : frameData(dim, 2, bits) { buildFrameCoeffs(); };
	linear(int dim, bitPerSubPixel_t bits, int numThreads) : frameData(dim, 2, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		auto ret = std::vector<float>(taps, 0.0f);
		ret[0] = 1 - x;
		ret[1] = x;
		return ret;
	}
};

struct cubic : public frameData {
	cubic(int dim, bitPerSubPixel_t bits) : frameData(dim, 4, bits) { buildFrameCoeffs(); };
	cubic(int dim, bitPerSubPixel_t bits, int numThreads) : frameData(dim, 4, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		auto ret = std::vector<float>(taps, 0.0f);
		ret[0] = -x * (x * (x - 2.0f) + 1.0f);
		ret[1] = std::powf(x, 2.0f) * (x - 2.0f) + 1.0f;
		ret[2] = -x * (x * (x - 1.0f) - 1.0f);
		ret[3] = std::powf(x, 2.0f) * (x - 1.0f);
		return ret;
	}
};

struct lanczos2 : public frameData {
	lanczos2(int dim, bitPerSubPixel_t bits) : frameData(dim, 4, bits) { buildFrameCoeffs(); };
	lanczos2(int dim, bitPerSubPixel_t bits, int numThreads) : frameData(dim, 4, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		auto ret = std::vector<float>(taps, 0.0f);

		if (x == 0.0f) {
			ret[-(taps / 2 - taps + 1)] = 1.0f;
			return ret;
		}

		auto deg2rad = [](float degrees) -> float {
			return degrees * std::numbers::pi / 180;
		};

		auto sum = 0.0f;
		for (unsigned int i = 0; i < ret.size(); i++)
			sum += ret[i] = (std::cos(deg2rad(i * -90.0f)) * std::sin((-x - 1.0f) * std::numbers::pi / 2.0f) +
						std::sin(deg2rad(i * -90.0f)) * std::cos((-x - 1.0f) * std::numbers::pi / 2.0f))
							/ std::powf(i - 1.0f - x, 2.0f);

		for (unsigned int i = 0; i < ret.size(); i++)
			ret[i] /= sum;

		return ret;
	}
};

struct lanczos3 : public frameData {

	lanczos3(int dim, bitPerSubPixel_t bits) : frameData(dim, 6, bits) { buildFrameCoeffs(); };
	lanczos3(int dim, bitPerSubPixel_t bits, int numThreads) : frameData(dim, 6, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		auto ret = std::vector<float>(taps, 0.0f);

		if (x == 0.0f) {
			ret[-(taps / 2 - taps + 1)] = 1.0f;
			return ret;
		}

		auto deg2rad = [](float degrees) -> float {
			return degrees * std::numbers::pi / 180;
		};

		auto sum = 0.0f;
		for (unsigned int i = 0; i < ret.size(); i++)
			sum += ret[i] = (std::cos(deg2rad(i * -120.0f)) * std::sin((-x - 2.0f) * std::numbers::pi / 3.0f) +
				std::sin(deg2rad(i * -120.0f)) * std::cos((-x - 2.0f) * std::numbers::pi / 3.0f))
			/ std::powf(i - 2.0f - x, 2.0f);

		for (unsigned int i = 0; i < ret.size(); i++)
			ret[i] /= sum;

		return ret;
	}
};

struct lanczos4 : public frameData {
	lanczos4(int dim, bitPerSubPixel_t bits) : frameData(dim, 8, bits) { buildFrameCoeffs(); };
	lanczos4(int dim, bitPerSubPixel_t bits, int numThreads) : frameData(dim, 8, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		auto ret = std::vector<float>(taps, 0.0f);

		if (x == 0.0f) {
			ret[-(taps / 2 - taps + 1)] = 1.0f;
			return ret;
		}

		auto deg2rad = [](float degrees) -> float {
			return degrees * std::numbers::pi / 180;
		};

		auto sum = 0.0f;
		for (unsigned int i = 0; i < ret.size(); i++)
			sum += ret[i] = (std::cos(deg2rad(i * -135.0f)) * std::sin((-x - 3.0f) * std::numbers::pi / 4.0f) +
				std::sin(deg2rad(i * -135.0f)) * std::cos((-x - 3.0f) * std::numbers::pi / 4.0f))
			/ std::powf(i - 3.0f - x, 2.0f);

		for (unsigned int i = 0; i < ret.size(); i++)
			ret[i] /= sum;

		return ret;
	}
};

struct lanczosN : public frameData {
	lanczosN(int dim, int taps, bitPerSubPixel_t bits) : frameData(dim, taps, bits) { buildFrameCoeffs(); };
	lanczosN(int dim, int taps, bitPerSubPixel_t bits, int numThreads) : frameData(dim, taps, bits, numThreads) { buildFrameCoeffs(); };

	std::vector<float> coeffsFunc(double x) final {
		auto ret = std::vector<float>(taps, 0.0f);

		if (x == 0.0f) {
			ret[-(taps / 2 - taps + 1)] = 1.0f;
			return ret;
		}

		auto deg2rad = [](float degrees) -> float {
			return degrees * std::numbers::pi / 180;
		};

		float halfTaps = taps / 2.0f;
		auto angle = (360.0f / (2.0f * halfTaps)) * (halfTaps - 1.0f);

		auto sum = 0.0f;
		for (unsigned int i = 0; i < ret.size(); i++)
			sum += ret[i] = (std::cos(deg2rad(i * -angle)) * std::sin((-x - halfTaps + 1.0f) * std::numbers::pi / (taps / 2)) +
				std::sin(deg2rad(i * -angle)) * std::cos((-x - halfTaps + 1.0f) * std::numbers::pi / (taps / 2)))
			/ std::powf(i - halfTaps + 1.0f - x, 2.0f);

		for (unsigned int i = 0; i < ret.size(); i++)
			ret[i] /= sum;

		return ret;
	}
};

#if INSTRSET >= 8 // AVX2
void centripetalCatMullRomInterpolation(typename frameData::lineData& self, const int i, const float* __restrict in, int* __restrict out) {

	const Vec8f Dj((self.len / 2) / (double)self.height);
	Vec8f x = Dj * (i + Vec8f(0, 1, 2, 3, 4, 5, 6, 7));
	Vec8f x_floor = floor(x);
	x -= x_floor;
	Vec8i x_int = truncatei(x_floor);

	Vec8f x_1 = lookup8(x_int - 1 - x_int[0], Vec8f().load(in + x_int[0]));
	Vec8f x0 = lookup8(x_int + 0 - x_int[0], Vec8f().load(in + x_int[0]));
	Vec8f x1 = lookup8(x_int + 1 - x_int[0], Vec8f().load(in + x_int[0]));
	Vec8f x2 = lookup8(x_int + 2 - x_int[0], Vec8f().load(in + x_int[0]));

	Vec8f res = [](Vec8f x0, Vec8f x1, Vec8f x2, Vec8f x3, Vec8f x) -> Vec8f {

		Vec8f t01 = approx_rsqrt(approx_rsqrt((x1 - x0) * (x1 - x0) + 1.0));
		Vec8f t12 = approx_rsqrt(approx_rsqrt((x2 - x1) * (x2 - x1) + 1.0));
		Vec8f t23 = approx_rsqrt(approx_rsqrt((x3 - x2) * (x3 - x2) + 1.0));
		Vec8f m1 = mul_add((x1 - x0) / t01 - (x2 - x0) / (t01 + t12), t12, x2 - x1);
		Vec8f m2 = mul_add((x3 - x2) / t23 - (x3 - x1) / (t12 + t23), t12, x2 - x1);
		Vec8f m12 = m1 + m2, x1_2 = x1 - x2;
		return mul_add(mul_add(2.0f, x1_2, m12) * x * x, x, mul_add(mul_sub(-3.0f, x1_2, m1 + m12) * x, x, mul_add(m1, x, x1)));

	}(x_1, x0, x1, x2, x);
	roundi(res).store(out + i);
}
#else
void centripetalCatMullRomInterpolation(typename frameData::lineData& self, const int i, const float* __restrict in, int* __restrict out) {

	const float Dj = ((self.len / 2) / (double)self.width);
	float x = Dj * i;
	float x_floor = std::floor(x);
	x -= x_floor;
	int x_int = x_floor;

	float x0 = in[x_int - 1];
	float x1 = in[x_int + 0];
	float x2 = in[x_int + 1];
	float x3 = in[x_int + 2];

	float t01 = std::pow((x1 - x0) * (x1 - x0) + 1.0, 0.25f);
	float t12 = std::pow((x2 - x1) * (x2 - x1) + 1.0, 0.25f);
	float t23 = std::pow((x3 - x2) * (x3 - x2) + 1.0, 0.25f);
	float m1 = (x2 - x1 + t12 * ((x1 - x0) / t01 - (x2 - x0) / (t01 + t12)));
	float m2 = (x2 - x1 + t12 * ((x3 - x2) / t23 - (x3 - x1) / (t12 + t23)));
	float res = (x1 + x * (m1 - x * (2.0 * m1 + m2 + 3.0 * x1 - 3.0 * x2 - x * (m1 + m2 + 2.0 * x1 - 2.0 * x2))));
	//float res = (2.0f * (x1 - x2) + m1 + m2) * x * x * x + (-3.0f * (x1 - x2) - m1 - m1 - m2) * x * x + (m1) * x + (x1);
	
	out[i] = std::round(res);
}
#endif