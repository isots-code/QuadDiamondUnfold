#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "interpLUTs.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {
	(void)argc;

	std::string input(argv[1]);

	try {
		auto decoder = ffmpegDecode(input);
		
		bool op = decoder.getOp();

		int interp = -1;
		if (argv[3])
			interp = std::atoi(argv[3]);

		int dim = decoder.getDim();

		AlignedVector<uint8_t> out0(dim * dim * 2 * 3);
		AlignedVector<uint8_t> out1(dim * dim * 2 * 3);

		std::fill(out0.begin(), out0.end(), 0);
		std::fill(out1.begin(), out1.end(), 0);

		frameData* dataLookup;
		switch (interp) {
			case 1:
				dataLookup = new nearest(op, dim, frameData::BITS_8);
				break;
			case 2:
				dataLookup = new linear(op, dim, frameData::BITS_8);
				break;
			case 3:
				dataLookup = new cubic(op, dim, frameData::BITS_8);
				break;
			case 4:
				dataLookup = new lanczos2(op, dim, frameData::BITS_8);
				break;
			case 5:
				dataLookup = new lanczos3(op, dim, frameData::BITS_8);
				break;
			case 6:
			case -1:
				dataLookup = new lanczos4(op, dim, frameData::BITS_8);
				break;
			case 7:
				dataLookup = new lanczosN(op, dim, std::atoi(argv[3]), frameData::BITS_8);
				break;
			case 8:
				dataLookup = new frameData(op, dim, 4, frameData::BITS_8, &centripetalCatMullRomInterpolation);
				break;
		}
		dataLookup->setOBuffers(out0.data(), out1.data());

		decoder.connectFrameData(*dataLookup);
		decoder.startDecode();
		decoder.startFFPlay(1);

		delete dataLookup;


	} catch (std::exception& e) {
		std::cerr << e.what();
		return -1;
	}

	return 0;
}