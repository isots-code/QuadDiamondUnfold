#pragma once

#include "frameData.h"

struct Compressor final : public frameData {

    struct lineDataCompressor;

    typedef void (*interpFunc_t)(Compressor::lineDataCompressor& self, const int x, const float* __restrict in, int* __restrict out);

    Compressor(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThreads);
    Compressor(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp);
    Compressor(int dim, int taps, bitPerSubPixel_t bits, int numThreads);
    Compressor(int dim, int taps, bitPerSubPixel_t bits);

    Compressor() = delete;

    ~Compressor() final;

    std::vector<float> coeffsFunc(double x) final;

    void kernel(const int id) final;

    struct lineDataCompressor final : frameData::lineData {

        lineDataCompressor(Compressor& parent, int y, int width);

        void processLine(const void* in, void* out) final;

    private:
        template<typename T>
        void gatherLines(const T* in);

        void interpLines(void);

        template<typename T>
        void storeLines(T* out);

        void constructGatherLUT(void) final;

        Compressor& parent;
    };


private:
    const interpFunc_t interp;
    std::vector<lineDataCompressor> linesCompressor;

};
