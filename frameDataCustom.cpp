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
        linesCustom[i].processLine(input, output);
};

frameDataCustom::lineDataCustom::lineDataCustom(frameDataCustom& parent, int y, int width) : frameData::lineData(parent, y, width), parent(parent) {}

void frameDataCustom::lineDataCustom::processLine(const void* in, void* out) {
    float inTopArray[3][paddedLen + taps];
    float inBotArray[3][paddedLen + taps];
    int outTopArray[3][width * 2];
    int outBotArray[3][width * 2];
    inTopLine = { inTopArray[0] + tapsOffset, inTopArray[1] + tapsOffset, inTopArray[2] + tapsOffset };
    inBotLine = { inBotArray[0] + tapsOffset, inBotArray[1] + tapsOffset, inBotArray[2] + tapsOffset };
    outTopLine = { outTopArray[0] + tapsOffset, outTopArray[1] + tapsOffset, outTopArray[2] + tapsOffset };
    outBotLine = { outBotArray[0] + tapsOffset, outBotArray[1] + tapsOffset, outBotArray[2] + tapsOffset };
    switch (this->parent.bitPerSubPixel) {
        case BITS_8:
            gatherLines(reinterpret_cast<const uint8_t*>(in));
            interpLines();
            storeLines(reinterpret_cast<uint8_t*>(out));
            break;
        case BITS_9:
        case BITS_10:
        case BITS_11:
        case BITS_12:
        case BITS_13:
        case BITS_14:
        case BITS_15:
        case BITS_16:
            gatherLines(reinterpret_cast<const uint16_t*>(in));
            interpLines();
            storeLines(reinterpret_cast<uint16_t*>(out));
            break;
    }
}

void frameDataCustom::lineDataCustom::interpLines(void) {
    for (int i = 0; i < width * 2; i += Vec8f::size()) {
        for (int component = 0; component < 3; component++) {
            parent.interp(*this, i, inTopLine[component], outTopLine[component]);
            parent.interp(*this, i, inBotLine[component], outBotLine[component]);
        }
    }
}

