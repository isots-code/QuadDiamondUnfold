#include "interpolators.h"

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

void centripetalCatMullRomInterpolation_AVX2(const int width, const int len, const int i, const float* __restrict in, int* __restrict out) {
	const Vec8f Dj(len / (double)width);
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
