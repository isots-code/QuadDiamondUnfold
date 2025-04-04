#include "frameData.h"

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

using namespace std::chrono;

template<>
void frameData::expandUV_AVX2(uint8_t* data, int width, int height) {

	struct wrapper {
		uint8_t* data;
		const size_t width;
		const size_t height;
		wrapper(uint8_t* data, const size_t width, const size_t height) : data(data), width(width), height(height) {};
		uint8_t& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * width + (height * width * comp)]; };
	};

	const Vec32uc LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15);

	//auto temp = new uint8_t[(width / 2) * (height / 2) * 2];
	//std::memcpy(temp, data, (width / 2) * (height / 2) * 2);
	//wrapper output(data, width, height);
	//wrapper input(temp, width / 2, height / 2);
	//for (int y = 0; y < height / 2; y++) {
	//	int x = 0;
	//	for (; x < (width / 2) - (Vec16uc::size() + 1); x += Vec16uc::size()) {
	//		for (int comp = 0; comp < 2; comp++) {
	//			Vec32uc in(Vec16uc().load(&input.at(x, y, comp)), Vec16uc());
	//			Vec32uc out(lookup32(LUT, in));
	//			out.store(&output.at(2 * x, 2 * y, comp));
	//			out.store(&output.at(2 * x, 2 * y + 1, comp));
	//		}
	//	}

	//	for (; x < width / 2; ++x) {
	//		for (int comp = 0; comp < 2; comp++)
	//			output.at(2 * x, 2 * y, comp) =
	//			output.at(2 * x + 1, 2 * y, comp) =
	//			output.at(2 * x, 2 * y + 1, comp) =
	//			output.at(2 * x + 1, 2 * y + 1, comp) =
	//			input.at(x, y, comp);
	//	}
	//}

	//delete[] temp;

	wrapper input(data, width / 2, height / 2);
	wrapper output(data, width, height);
	for (int comp = 1; comp >= 0; comp--) {
		for (int y = height / 2 - 1; y >= 0; y--) {
			int x = width / 2 - Vec16uc().size();
			for (; x >= 0; x -= Vec16uc().size()) {
				Vec32uc in(Vec16uc().load(&input.at(x, y, comp)), Vec16uc());
				Vec32uc out(lookup32(LUT, in));
				out.store(&output.at(2 * x, 2 * y, comp));
				out.store(&output.at(2 * x, 2 * y + 1, comp));
			//}
			}

			for (x += x < 0 ? Vec16uc().size() : 0; x >= 0; x--) {
				//for (int comp = 1; comp >= 0; comp--) {
				output.at(2 * x, 2 * y, comp) =
					output.at(2 * x + 1, 2 * y, comp) =
					output.at(2 * x, 2 * y + 1, comp) =
					output.at(2 * x + 1, 2 * y + 1, comp) =
					input.at(x, y, comp);
			}
		}
	}

}

template<>
void frameData::expandUV_AVX2(uint16_t* data, int width, int height) {

	struct wrapper {
		uint16_t* data;
		const size_t width;
		const size_t height;
		wrapper(uint16_t* data, const size_t width, const size_t height) : data(data), width(width), height(height) {};
		uint16_t& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * width + (height * width * comp)]; };
	};

	auto temp = new uint16_t[(width / 2) * (height / 2) * 2];
	std::memcpy(temp, data, (width / 2) * (height / 2) * 2);
	wrapper input(temp, width / 2, height / 2);
	wrapper output(data, width, height);

	const Vec16us LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7);

	for (int y = 0; y < height / 2; y++) {
		int x = 0;
		for (; x < (width / 2) - (Vec8us::size() + 1); x += Vec8us::size()) {
			for (int comp = 0; comp < 2; comp++) {
				Vec16us in(Vec8us().load(&input.at(x, y, comp)), Vec8us());
				Vec16us out(lookup16(LUT, in));
				out.store(&output.at(2 * x, 2 * y, comp));
				out.store(&output.at(2 * x, 2 * y + 1, comp));
			}
		}

		for (; x < width / 2; ++x) {
			for (int comp = 0; comp < 2; comp++)
				output.at(2 * x, 2 * y, comp) =
				output.at(2 * x + 1, 2 * y, comp) =
				output.at(2 * x, 2 * y + 1, comp) =
				output.at(2 * x + 1, 2 * y + 1, comp) =
				input.at(x, y, comp);
		}
	}

	delete[] temp;

}
