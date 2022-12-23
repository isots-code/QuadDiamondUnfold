#pragma once

#include "frameData.h"
#include "extras.h"

template <typename T, typename U, bool NT>
struct frameDataCustom : public frameData<T, U, NT> {

    struct lineDataCustom;

    typedef void (*interpFunc_t)(frameDataCustom::lineDataCustom& self, const int x, const float* __restrict in, float* __restrict out);

    frameDataCustom(int dim, int taps, interpFunc_t interp, int numThreads = 1);

    frameDataCustom() = delete;

    ~frameDataCustom();

    void kernel(const int id) final;

    struct lineDataCustom : frameData<T, U, NT>::lineData {

        lineDataCustom(frameDataCustom& parent, int y, int width);

        void processLine(const T* in, T* out) final;

    private:
        void interpLines(void) final;

        frameDataCustom& parent;
    };


private:
    const interpFunc_t interp;
    std::vector<lineDataCustom> linesCustom;

};

template <typename T, typename U, bool NT>
frameDataCustom<T, U, NT>::frameDataCustom(int dim, int taps, interpFunc_t interp, int numThreads) : frameData<T, U, NT>(dim, taps, numThreads), interp(interp) {
    linesCustom.reserve(dim / 2);
    for (int i = 0; i < dim / 2; i++)
        linesCustom.emplace_back(*this, i, dim);
}

template <typename T, typename U, bool NT>
frameDataCustom<T, U, NT>::~frameDataCustom() {
    this->stop();
    linesCustom.clear();
}

template <typename T, typename U, bool NT>
void frameDataCustom<T, U, NT>::kernel(const int id) {
    for (size_t i = id; i < dim / 2; i += this->numThreads) // topo e fundo por iteração
        linesCustom[i].processLine(this->input, this->output);
};

template <typename T, typename U, bool NT>
frameDataCustom<T, U, NT>::lineDataCustom::lineDataCustom(frameDataCustom& parent, int y, int width) : frameData<T, U, NT>::lineData(parent, y, width), parent(parent) {}

template <typename T, typename U, bool NT>
void frameDataCustom<T, U, NT>::lineDataCustom::processLine(const T* in, T* out) {
    float inTopArray[3][paddedLen + taps];
    float inBotArray[3][paddedLen + taps];
    U outTopArray[3][width * 2];
    U outBotArray[3][width * 2];
    inTopLine = { inTopArray[0] + tapsOffset, inTopArray[1] + tapsOffset, inTopArray[2] + tapsOffset };
    inBotLine = { inBotArray[0] + tapsOffset, inBotArray[1] + tapsOffset, inBotArray[2] + tapsOffset };
    outTopLine = { outTopArray[0] + tapsOffset, outTopArray[1] + tapsOffset, outTopArray[2] + tapsOffset };
    outBotLine = { outBotArray[0] + tapsOffset, outBotArray[1] + tapsOffset, outBotArray[2] + tapsOffset };
    gatherLines(in);
    interpLines();
    storeLines(out);
}

template <typename T, typename U, bool NT>
void frameDataCustom<T, U, NT>::lineDataCustom::interpLines(void) {
    for (int i = 0; i < width * 2; i += Vec8f::size()) {
        for (int component = 0; component < 3; component++) {
            parent.interp(*this, i, inTopLine[component], outTopLine[component]);
            parent.interp(*this, i, inBotLine[component], outBotLine[component]);
        }
    }
}

template <typename T = uint8_t, typename U = float, bool NT = false>
void centripetalCatMullRomInterpolation(typename frameDataCustom<T, U, NT>::lineDataCustom& self, const int i, const float* __restrict in, float* __restrict out) {

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

    [](Vec8f x0, Vec8f x1, Vec8f x2, Vec8f x3, Vec8f x) -> Vec8f {

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

    }(x_1, x0, x1, x2, x).store(out + i);
}