#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "interpLUTs.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {

	try {
		auto decoder = ffmpegDecode(std::string(argv[1]));

		int dim = decoder.getDim();

		AlignedVector<uint8_t> out0(dim * dim * 2 * 3);
		AlignedVector<uint8_t> out1(dim * dim * 2 * 3);

		frameData* dataLookup;
		//dataLookup = new frameDataCustom(dim, 4, frameData::BITS_8, &centripetalCatMullRomInterpolation);
		//dataLookup = new lanczosN(dim, 24, frameData::BITS_8);
		//dataLookup = new lanczosN(dim, 10, frameData::BITS_8);
		//dataLookup = new lanczosN(dim, 8, frameData::BITS_8);
		dataLookup = new lanczos4(dim, frameData::BITS_8);
		//dataLookup = new lanczos3(dim, frameData::BITS_8);
		//dataLookup = new lanczos2(dim, frameData::BITS_8);
		//dataLookup = new cubic(dim, frameData::BITS_8);
		//dataLookup = new linear(dim, frameData::BITS_8);
		//dataLookup = new nearest(dim, frameData::BITS_8);
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