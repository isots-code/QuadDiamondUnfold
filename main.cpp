#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "frameData.h"
#include "frameDataCustom.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {

	try {
		auto decoder = ffmpegDecode(std::string(argv[1]));

		int dim = decoder.getDim();

		AlignedVector<uint8_t> out0(dim * dim * 2 * 3);
		AlignedVector<uint8_t> out1(dim * dim * 2 * 3);

		frameData* dataLookup;
		dataLookup = new frameDataCustom(dim, 4, frameData::BITS_8, &centripetalCatMullRomInterpolation);
		//dataLookup = new frameData(dim, 4, frameData::BITS_8);
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