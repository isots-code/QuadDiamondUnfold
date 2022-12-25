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

void centripetalCatMullRomInterpolation(typename frameDataCustom::lineDataCustom& self, const int i, const float* __restrict in, int* __restrict out) {

    const Vec8f Dj((self.len / 2) / (double)self.width);
    Vec8f x = Dj * (i + Vec8f(0, 1, 2, 3, 4, 5, 6, 7));
    Vec8f x_floor = floor(x);
    x -= x_floor;
    Vec8i x_int = truncatei(x_floor);

    Vec8f x_1 = lookup8(x_int - 1 - x_int[0], Vec8f().load(in + x_int[0]));
    Vec8f x0 = lookup8(x_int + 0 - x_int[0], Vec8f().load(in + x_int[0]));
    Vec8f x1 = lookup8(x_int + 1 - x_int[0], Vec8f().load(in + x_int[0]));
    Vec8f x2 = lookup8(x_int + 2 - x_int[0], Vec8f().load(in + x_int[0]));

    Vec8f res = [](Vec8f x0, Vec8f x1, Vec8f x2, Vec8f x3, Vec8f x) -> Vec8f {

        Vec8f t01 = approx_rsqrt(approx_rsqrt((x1 - x0) * (x1 - x0) + 1.0));
        Vec8f t12 = approx_rsqrt(approx_rsqrt((x2 - x1) * (x2 - x1) + 1.0));
        Vec8f t23 = approx_rsqrt(approx_rsqrt((x3 - x2) * (x3 - x2) + 1.0));
        Vec8f m1 = mul_add((x1 - x0) / t01 - (x2 - x0) / (t01 + t12), t12, x2 - x1);
        Vec8f m2 = mul_add((x3 - x2) / t23 - (x3 - x1) / (t12 + t23), t12, x2 - x1);
        Vec8f m12 = m1 + m2, x1_2 = x1 - x2;
        return mul_add(mul_add(2.0f, x1_2, m12) * x * x, x, mul_add(mul_sub(-3.0f, x1_2, m1 + m12) * x, x, mul_add(m1, x, x1)));

    }(x_1, x0, x1, x2, x);
    roundi(res).store(out + i);
}