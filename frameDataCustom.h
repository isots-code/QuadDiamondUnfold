#pragma once

#include "frameData.h"
#include "extras.h"

struct frameDataCustom final : public frameData {

    struct lineDataCustom;

    typedef void (*interpFunc_t)(frameDataCustom::lineDataCustom& self, const int x, const float* __restrict in, int* __restrict out);

    frameDataCustom(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThreads);
    frameDataCustom(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp);

    frameDataCustom() = delete;

    virtual ~frameDataCustom() final;

    void kernel(const int id) final;

    struct lineDataCustom final : frameData::lineData {

        lineDataCustom(frameDataCustom& parent, int y, int width);

        void processLine(const void* in, void* out) final;

    private:
        void interpLines(void) final;

        frameDataCustom& parent;
    };


private:
    const interpFunc_t interp;
    std::vector<lineDataCustom> linesCustom;

};
frameDataCustom::frameDataCustom(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThreads) : frameData(dim, taps, bits, numThreads), interp(interp) {
    linesCustom.reserve(dim / 2);
    for (int i = 0; i < dim / 2; i++)
        linesCustom.emplace_back(*this, i, dim);
}

frameDataCustom::frameDataCustom(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp) : frameDataCustom(dim, taps, bits, interp, std::thread::hardware_concurrency()) { }

frameDataCustom::~frameDataCustom() {
    this->stop();
    linesCustom.clear();
}

void frameDataCustom::kernel(const int id) {
    for (int i = id; i < this->dim / 2; i += this->numThreads) // topo e fundo por iteração
        linesCustom[i].processLine(this->input, this->output);
};

frameDataCustom::lineDataCustom::lineDataCustom(frameDataCustom& parent, int y, int width) : frameData::lineData(parent, y, width), parent(parent) {}

void frameDataCustom::lineDataCustom::processLine(const void* in, void* out) {
    float inTopArray[3][this->paddedLen + this->taps];
    float inBotArray[3][this->paddedLen + this->taps];
    int outTopArray[3][this->width * 2];
    int outBotArray[3][this->width * 2];
    this->inTopLine = { inTopArray[0] + this->tapsOffset, inTopArray[1] + this->tapsOffset, inTopArray[2] + this->tapsOffset };
    this->inBotLine = { inBotArray[0] + this->tapsOffset, inBotArray[1] + this->tapsOffset, inBotArray[2] + this->tapsOffset };
    this->outTopLine = { outTopArray[0] + this->tapsOffset, outTopArray[1] + this->tapsOffset, outTopArray[2] + this->tapsOffset };
    this->outBotLine = { outBotArray[0] + this->tapsOffset, outBotArray[1] + this->tapsOffset, outBotArray[2] + this->tapsOffset };
    switch (this->parent.bitPerSubPixel) {
        case BITS_8:
            this->gatherLines(reinterpret_cast<const uint8_t*>(in));
            break;
        case BITS_9:
        case BITS_10:
        case BITS_11:
        case BITS_12:
        case BITS_13:
        case BITS_14:
        case BITS_15:
        case BITS_16:
            this->gatherLines(reinterpret_cast<const uint16_t*>(in));
            break;
    }
    interpLines();
    switch (this->parent.bitPerSubPixel) {
        case BITS_8:
            this->storeLines(reinterpret_cast<uint8_t*>(out));
            break;
        case BITS_9:
        case BITS_10:
        case BITS_11:
        case BITS_12:
        case BITS_13:
        case BITS_14:
        case BITS_15:
        case BITS_16:
            this->storeLines(reinterpret_cast<uint16_t*>(out));
            break;
    }
}

void frameDataCustom::lineDataCustom::interpLines(void) {
    for (int i = 0; i < this->width * 2; i += Vec8f::size()) {
        for (int component = 0; component < 3; component++) {
            parent.interp(*this, i, this->inTopLine[component], this->outTopLine[component]);
            parent.interp(*this, i, this->inBotLine[component], this->outBotLine[component]);
        }
    }
}

template <typename T = uint8_t>
void centripetalCatMullRomInterpolation(typename frameDataCustom::lineDataCustom& self, const int i, const float* __restrict in, int* __restrict out) {

    const Vec8f Dj((self.len / 2) / (double)self.width);
    Vec8f x = Dj * (i + Vec8f(0, 1, 2, 3, 4, 5, 6, 7));
    Vec8f x_floor = floor(x);
    x -= x_floor;
    Vec8i x_int = truncatei(x_floor);

    auto circular = [self](int offset, Vec8i indexes) -> Vec8i {
        Vec8ib mask = indexes + offset >= self.len;
        mask |= indexes + offset < 0;
        return vecModCond(indexes + offset, mask, self.len);
    };

    Vec8i ret_1 = circular(-1, x_int);
    Vec8i ret0 = circular(0, x_int);
    Vec8i ret1 = circular(1, x_int);
    Vec8i ret2 = circular(2, x_int);

    Vec8f x_1 = gather(in, ret_1);
    Vec8f x0 = gather(in, ret0);
    Vec8f x1 = gather(in, ret1);
    Vec8f x2 = gather(in, ret2);

    Vec8f res = [](Vec8f x0, Vec8f x1, Vec8f x2, Vec8f x3, Vec8f x) -> Vec8f {

        Vec8f t01 = approx_rsqrt(approx_rsqrt((x1 - x0) * (x1 - x0) + 1.0));
        Vec8f t12 = approx_rsqrt(approx_rsqrt((x2 - x1) * (x2 - x1) + 1.0));
        Vec8f t23 = approx_rsqrt(approx_rsqrt((x3 - x2) * (x3 - x2) + 1.0));
        Vec8f m1 = mul_add((x1 - x0) / t01 - (x2 - x0) / (t01 + t12), t12, x2 - x1);
        Vec8f m2 = mul_add((x3 - x2) / t23 - (x3 - x1) / (t12 + t23), t12, x2 - x1);
        Vec8f m12 = m1 + m2, x1_2 = x1 - x2;
        return mul_add(mul_add(2.0f, x1_2, m12) * x * x, x, mul_add(mul_sub(-3.0f, x1_2, m1 + m12) * x, x, mul_add(m1, x, x1)));
        
        //Vec8f t01 = pow_ratio((x1 - x0) * (x1 - x0) + 1.0, 1, 4);
        //Vec8f t12 = pow_ratio((x2 - x1) * (x2 - x1) + 1.0, 1, 4);
        //Vec8f t23 = pow_ratio((x3 - x2) * (x3 - x2) + 1.0, 1, 4);
        //Vec8f m1 = (x2 - x1 + t12 * ((x1 - x0) / t01 - (x2 - x0) / (t01 + t12)));
        //Vec8f m2 = (x2 - x1 + t12 * ((x3 - x2) / t23 - (x3 - x1) / (t12 + t23)));
        //return (x1 + x * (m1 - x * (2.0 * m1 + m2 + 3.0 * x1 - 3.0 * x2 - x * (m1 + m2 + 2.0 * x1 - 2.0 * x2))));
        //return (2.0f * (x1 - x2) + m1 + m2) * x * x * x + (-3.0f * (x1 - x2) - m1 - m1 - m2) * x * x + (m1) * x + (x1);

    }(x_1, x0, x1, x2, x);
    roundi(res).store(out + i);
}