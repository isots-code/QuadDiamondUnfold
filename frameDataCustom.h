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

void centripetalCatMullRomInterpolation(typename frameDataCustom::lineDataCustom& self, const int i, const float* __restrict in, int* __restrict out);