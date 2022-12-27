#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "interpLUTs.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {
	(void)argc;

	try {
		auto decoder = ffmpegDecode(std::string(argv[1]));

		int dim = decoder.getDim();

		AlignedVector<uint8_t> out0(dim * dim * 2 * 3);
		AlignedVector<uint8_t> out1(dim * dim * 2 * 3);

		frameData* dataLookup;
		if (argv[2] == nullptr)
			dataLookup = new lanczos4(dim, frameData::BITS_8, 1);
		else {
			switch (std::atoi(argv[2])) {
				case 1:
					dataLookup = new nearest(dim, frameData::BITS_8);
					break;
				case 2:
					dataLookup = new linear(dim, frameData::BITS_8);
					break;
				case 3:
					dataLookup = new cubic(dim, frameData::BITS_8);
					break;
				case 4:
					dataLookup = new lanczos2(dim, frameData::BITS_8);
					break;
				case 5:
					dataLookup = new lanczos3(dim, frameData::BITS_8);
					break;
				case 6:
					dataLookup = new lanczos4(dim, frameData::BITS_8);
					break;
				case 7:
					dataLookup = new lanczosN(dim, std::atoi(argv[3]), frameData::BITS_8);
					break;
				//case 8:
					dataLookup = new frameData(dim, 4, frameData::BITS_8, &centripetalCatMullRomInterpolation);
				//	break;
			}
		}
		dataLookup->setOBuffers(out0.data(), out1.data());

		decoder.connectFrameData(*dataLookup);
		decoder.startDecode();
		decoder.startFFPlay();

		delete dataLookup;


	} catch (std::exception& e) {
		std::cerr << e.what();
		return -1;
	}

	return 0;
}