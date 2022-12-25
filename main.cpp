#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>

#include <windows.h>
#include <process.h>

#include "frameData.h"
#include "frameDataCustom.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {

	using bitType = uint8_t;
	const auto bitDepth = frameData::BITS_8;

	try {
		auto decoder = ffmpegDecode(std::string(argv[1]), bitDepth);

		int dim = decoder.getDim();

		AlignedVector<bitType> out0(dim * dim * 2 * 3);
		AlignedVector<bitType> out1(dim * dim * 2 * 3);

		//auto dataLookup = frameDataCustom(dim, 4, &centripetalCatMullRomInterpolation);
		auto dataLookup = frameData(dim, 4, bitDepth);
		dataLookup.setOBuffers(out0.data(), out1.data());

		decoder.connectFrameData(dataLookup);
		decoder.startDecode();
		decoder.startFFPlay();

		
	} catch (std::exception& e) {
		std::cerr << e.what();
		return -1;
	}

	return 0;
}