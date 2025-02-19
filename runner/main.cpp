#include <iostream>

#include "frameData.h"
#include "interpolators.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {
	(void)argc;

	std::string input(argv[1]);

	try {
		auto decoder = ffmpegDecode(input, true);
		
		bool op = decoder.getOp();

		int interpChoice = -1;
		if (argv[2])
			interpChoice = std::atoi(argv[2]);

		int dim = decoder.getDim();

		AlignedVector<uint8_t> out0(dim * dim * 2 * 3);
		AlignedVector<uint8_t> out1(dim * dim * 2 * 3);

		frameData* dataLookup;
		if (interpChoice == -1)
			dataLookup = new frameData(op, dim, frameData::BITS_8);
		else if (interpChoice >= interpolator::NEAREST && interpChoice <= interpolator::LANCZOSN)
			dataLookup = new frameData(op, dim, frameData::BITS_8, interpolators[interpChoice - interpolator::NEAREST]);
		else if (interpChoice >= interpolator::CENTRIPETAL_CATMULL_ROM && interpChoice <= interpolator::CENTRIPETAL_CATMULL_ROM)
			dataLookup = new frameData(op, dim, frameData::BITS_8, customInterpolators[interpChoice - interpolator::CENTRIPETAL_CATMULL_ROM]);
		else 
			throw std::runtime_error("Unsupported interpolator");
		dataLookup->setOBuffers(out0.data(), out1.data());

		decoder.connectFrameData(*dataLookup);
		decoder.start();

		while (decoder.running) {
			using namespace std::chrono;
			std::this_thread::sleep_for(100ms);
		}

		delete dataLookup;

	} catch (std::exception& e) {
		std::cerr << e.what();
		return -1;
	}

	return 0;
}
