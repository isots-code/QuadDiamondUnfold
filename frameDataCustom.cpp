#include "frameDataCustom.h"

frameDataCustom::frameDataCustom(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThreads) : frameData(dim, taps, bits, numThreads), interp(interp) {
    linesCustom.reserve(dim / 2);
    for (int i = 0; i < dim / 2; i++)
        linesCustom.emplace_back(*this, i, dim);
}

frameDataCustom::frameDataCustom(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp) : frameDataCustom(dim, taps, bits, interp, std::thread::hardware_concurrency()) {}

frameDataCustom::~frameDataCustom() {
    stop();
    linesCustom.clear();
}

std::vector<float> frameDataCustom::coeffsFunc(double x) {
    (void)x;
    return std::vector<float>(taps, 1.0f);
}

void frameDataCustom::kernel(const int id) {
    for (int i = id; i < dim / 2; i += numThreads) // topo e fundo por iteração
        linesCustom[i].processLine(this->input, this->output);
};

frameDataCustom::lineDataCustom::lineDataCustom(frameDataCustom& parent, int y, int width) : frameData::lineData(parent, y, width), parent(parent) {}

void frameDataCustom::lineDataCustom::interpLines(void) {

#if INSTRSET >= 8 // AVX2
    int increment = Vec8f::size();
#else
    int increment = 1;
#endif

    for (int i = 0; i < width * 2; i += increment) {
        for (int component = 0; component < 3; component++) {
            parent.interp(*this, i, inTopLine[component], outTopLine[component]);
            parent.interp(*this, i, inBotLine[component], outBotLine[component]);
        }
    }
}

