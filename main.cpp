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

	using bitDepth = bits8_t;

	try {
		auto decoder = ffmpegDecode<bitDepth>(std::string(argv[1]));

		int dim = decoder.getDim();

		AlignedVector<bitDepth> out0(dim * dim * 2 * 3);
		AlignedVector<bitDepth> out1(dim * dim * 2 * 3);

		//auto dataLookup = frameDataCustom<bitDepth>(dim, 4, &centripetalCatMullRomInterpolation<bitDepth>);
		auto dataLookup = frameData<bitDepth>(dim, 4);
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