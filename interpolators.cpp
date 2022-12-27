#include "interpolators.h"

interp_t interpolators[] = {
   { nearest, 1 },
   { linear, 2 },
   { cubic, 4 },
   { lanczos2, 4 },
   { lanczos3, 6 },
   { lanczos4, 8 },
   { lanczosN, -1 }
};

customInterp_t customInterpolators[]{
	{ centripetalCatMullRomInterpolation, 4},
	{ centripetalCatMullRomInterpolation_AVX2, 4}
};

std::vector<float> nearest(double x, int taps) {
	(void)x;
	(void)taps;
	return std::vector<float>({ 1.0f });
}


std::vector<float> linear(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0f);
	ret[0] = 1 - x;
	ret[1] = x;
	return ret;
}

std::vector<float> cubic(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0f);
	ret[0] = -x * (x * (x - 2.0f) + 1.0f);
	ret[1] = std::powf(x, 2.0f) * (x - 2.0f) + 1.0f;
	ret[2] = -x * (x * (x - 1.0f) - 1.0f);
	ret[3] = std::powf(x, 2.0f) * (x - 1.0f);
	return ret;
}

std::vector<float> lanczos2(double x, int taps) {
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

std::vector<float> lanczos3(double x, int taps) {
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

std::vector<float> lanczos4(double x, int taps) {
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

std::vector<float> lanczosN(double x, int taps) {
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

void centripetalCatMullRomInterpolation(const int width, const int len, const int i, const float* __restrict in, int* __restrict out) {
	const float Dj = len / (double)width;
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
