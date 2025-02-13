#ifndef OG_H
#define OG_H

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdbool>
#include <cmath>

#include <benchmark/benchmark.h>

#define clamp8bit(in) (uint8_t)((in > 255 ? 255 : in < 0 ? 0 : in) + 0.5)
#define circular(a, b) (((a < 0 ? b : 0) + (a % b)) % b)
#define PI 3.14159265358979323846264338327950288419716939937510
#define s60 0.8660254037844386467637231707529361834714026269051
#define s45 0.7071067811865475244008443621048490392848359376884

char (*interpolador) (char*, double, int);

static char interpol_nn(char* image, double x, int N) {

    int x_int = round(x);
    return *(image + circular(x_int, N));

}

static char interpol_lin(char* image, double x, int N) {

    int x_int = floor(x);
    x -= x_int;
    uint8_t x1 = *(image + circular(x_int, N));
    uint8_t x2 = *(image + circular(x_int + (x > 0), N));

    return x * (x2 - x1) + x1 + 0.5;

}

static char interpol_cub(char* image, double x, int N) {

    int x_int = floor(x);
    x -= x_int;
    bool n0 = x > 0;
    uint8_t x1 = *(image + circular(x_int, N));
    if (!n0) return x1;
    uint8_t x2 = *(image + circular(x_int + n0, N));
    uint8_t x0 = *(image + circular(x_int - 1, N));
    uint8_t x3 = *(image + circular(x_int + n0 + 1, N));

    return clamp8bit(x1 - x * (x0 - x2 + x * (2 * x1 - 2 * x0 - x2 + x3 + x * (x0 - x1 + x2 - x3)))); //cubic

}

static char interpol_lanczos2(char* image, double x, int N) {

    static const double cs[4][2] = {
        {1, 0},
        {0, -1},
        {-1, 0},
        {0, 1}
    };

    double sum_coeffs = 0, sum = 0, coeffs[4];
    int x_int = floor(x);
    uint8_t xx[4];
    x -= x_int;
    bool n0 = x > 0;
    xx[1] = *(image + circular(x_int, N));
    if (!n0) return xx[1];
    xx[2] = *(image + circular(x_int + n0, N));
    xx[0] = *(image + circular(x_int - 1, N));
    xx[3] = *(image + circular(x_int + n0 + 1, N));

    double aux = -1 - x, s_aux = sin(aux * PI / 2), c_aux = cos(aux * PI / 2);

    for (int i = 0; i < 4; i++) sum_coeffs += coeffs[i] = (cs[i][0] * s_aux + cs[i][1] * c_aux) / (i - 1 - x) / (i - 1 - x);

    for (int i = 0; i < 4; i++) sum += xx[i] * coeffs[i] / sum_coeffs;

    return clamp8bit(sum);

}

static char interpol_lanczos3(char* image, double x, int N) {

    static const double cs[][2] = {
        {1, 0},
        {-0.5, -s60},
        {-0.5, s60},
        {1, 0},
        {-0.5, -s60},
        {-0.5, s60},
    };

    double sum_coeffs = 0, sum = 0, coeffs[6];
    int x_int = floor(x);
    uint8_t xx[6];
    x -= x_int;
    bool n0 = x > 0;
    xx[2] = *(image + circular(x_int, N));
    if (!n0) return xx[2];
    xx[3] = *(image + circular(x_int + n0, N));
    xx[1] = *(image + circular(x_int - 1, N));
    xx[4] = *(image + circular(x_int + n0 + 1, N));
    xx[0] = *(image + circular(x_int - 2, N));
    xx[5] = *(image + circular(x_int + n0 + 2, N));

    double aux = -2 - x, s_aux = sin(aux * PI / 3), c_aux = cos(aux * PI / 3);

    for (int i = 0; i < 6; i++) sum_coeffs += coeffs[i] = (cs[i][0] * s_aux + cs[i][1] * c_aux) / (i - 2 - x) / (i - 2 - x);

    for (int i = 0; i < 6; i++) sum += xx[i] * coeffs[i] / sum_coeffs;

    return clamp8bit(sum);

}

static char interpol_lanczos4(char* image, double x, int N) {

    static const double cs[][2] = {
        {1, 0},
        {-s45, -s45},
        {0, 1},
        {s45, -s45},
        {-1, 0},
        {s45, s45},
        {0, -1},
        {-s45, s45}
    };

    double sum_coeffs = 0, sum = 0, coeffs[8];
    int x_int = floor(x);
    uint8_t xx[8];
    x -= x_int;
    bool n0 = x > 0;
    xx[3] = *(image + circular(x_int, N));
    if (!n0) return xx[3];
    xx[4] = *(image + circular(x_int + n0, N));
    xx[2] = *(image + circular(x_int - 1, N));
    xx[5] = *(image + circular(x_int + n0 + 1, N));
    xx[1] = *(image + circular(x_int - 2, N));
    xx[6] = *(image + circular(x_int + n0 + 2, N));
    xx[0] = *(image + circular(x_int - 3, N));
    xx[7] = *(image + circular(x_int + n0 + 3, N));

    double aux = -3 - x, s_aux = sin(aux * PI / 4), c_aux = cos(aux * PI / 4);

    for (int i = 0; i < 8; i++) sum_coeffs += coeffs[i] = (cs[i][0] * s_aux + cs[i][1] * c_aux) / (i - 3 - x) / (i - 3 - x);

    for (int i = 0; i < 8; i++) sum += xx[i] * coeffs[i] / sum_coeffs;

    return clamp8bit(sum);

}

static char interpol_catmull_rom(char* image, double x, int N) {

    int x_int = floor(x);
    x -= x_int;
    bool n0 = x > 0;
    uint8_t x1 = *(image + circular(x_int, N));
    if (!n0) return x1;
    uint8_t x2 = *(image + circular(x_int + n0, N));
    uint8_t x0 = *(image + circular(x_int - 1, N));
    uint8_t x3 = *(image + circular(x_int + n0 + 1, N));

    return clamp8bit(x1 + (x * (x2 - x0 + x * (x0 * 2 - 5 * x1 + x2 * 4 - x3 + x * (3 * (x1 - x2) + x3 - x0)))) / 2); //catmull
}

static char interpol_centri_catmull_rom(char* image, double x, int N) {

    int x_int = floor(x);
    x -= x_int;
    bool n0 = x > 0;
    uint8_t x1 = *(image + circular(x_int, N));
    if (!n0) return x1;
    uint8_t x2 = *(image + circular(x_int + n0, N));
    uint8_t x0 = *(image + circular(x_int - 1, N));
    uint8_t x3 = *(image + circular(x_int + n0 + 1, N));

    double t01 = pow(pow(abs(x1 - x0), 2) + 1, 0.25);
    double t12 = pow(pow(abs(x2 - x1), 2) + 1, 0.25);
    double t23 = pow(pow(abs(x3 - x2), 2) + 1, 0.25);

    double m1 = (x2 - x1 + t12 * ((x1 - x0) / t01 - (x2 - x0) / (t01 + t12)));
    double m2 = (x2 - x1 + t12 * ((x3 - x2) / t23 - (x3 - x1) / (t12 + t23)));

    return clamp8bit(x1 + x * (m1 - x * (2 * m1 + m2 + 3 * x1 - 3 * x2 - x * (m1 + m2 + 2 * x1 - 2 * x2))));
}

static void conversao_invY(char* in, int N) {

    int x;
    int y;
    int n2 = 2 * N;
    double dist_y;
    double dist_x;
    double dist_Lj;
    double novo_x;
    double novo_y;
    double Lj;
    double Dj;
    double tx;
    char* rx;
    char* linhaY = (char*)std::calloc(n2, 1);
    char* out = (char*)std::calloc(n2 * N, 1);
    memset(out, 128, n2 * N);

    for (y = 0; y < N; y++) {

        Lj = (N - abs(2 * y - N + 1)) * 2;
        Dj = n2 / Lj;

        for (x = 0; x < Lj; x++) {

            dist_x = x - Lj / 2;
            dist_y = (N - abs(2 * y - N + 1) - 1) / 2;
            dist_Lj = (Lj - abs(2 * x - Lj + 1) - 1) / 2;

            novo_x = (dist_x > dist_y ? dist_y : (dist_x + 1 < -dist_y ? -dist_y - 1 : dist_x)) + N / 2;
            novo_y = abs(dist_x) > dist_y ? (y < N / 2 ? dist_Lj : N - dist_Lj - 1) : y;

            *(linhaY + x) = *(in + (int)novo_x + (int)novo_y * N);

        }

        for (x = 0; x < n2; x++) {

            tx = (x + 1) / Dj - 1;
            rx = x + y * n2 + out;

            *(rx) = interpolador(linhaY, tx, Lj);

        }

    }

    memcpy(in, out, n2 * N);
    free(out);
    free(linhaY);

}

static void conversao_invUV(char* in, int N) {

    int x;
    int y;
    int n2 = 2 * N;
    int nn = N * N;
    double dist_y;
    double dist_x;
    double dist_Lj;
    double novo_x;
    double novo_y;
    double Lj;
    double Dj;
    double tx;
    char* rx;
    char* linhaU = (char*)std::calloc(n2, 1);
    char* linhaV = (char*)std::calloc(n2, 1);
    char* out = (char*)std::calloc(nn * 4, 1);
    memset(out, 128, nn * 4);

    for (y = 0; y < N; y++) {

        Lj = (N - abs(2 * y - N + 1)) * 2;
        Dj = n2 / Lj;

        for (x = 0; x < Lj; x++) {

            dist_x = x - Lj / 2;
            dist_y = (N - abs(2 * y - N + 1) - 1) / 2;
            dist_Lj = (Lj - abs(2 * x - Lj + 1) - 1) / 2;

            novo_x = (dist_x > dist_y ? dist_y : (dist_x + 1 < -dist_y ? -dist_y - 1 : dist_x)) + N / 2;
            novo_y = abs(dist_x) > dist_y ? (y < N / 2 ? dist_Lj : N - dist_Lj - 1) : y;

            *(linhaU + x) = *(in + (int)novo_x + (int)novo_y * N);
            *(linhaV + x) = *(in + (int)novo_x + (int)novo_y * N + nn);

        }

        for (x = 0; x < n2; x++) {

            tx = (x + 1) / Dj - 1;
            rx = x + y * n2 + out;

            *(rx) = interpolador(linhaU, tx, Lj);
            *(rx + n2 * N) = interpolador(linhaV, tx, Lj);

        }

    }

    memcpy(in + nn * 4, out, nn * 4);
    free(out);
    free(linhaU);
    free(linhaV);

}

char (*inter[])(char*, double, int) = { interpol_nn, interpol_lin, interpol_cub, interpol_lanczos2, interpol_lanczos3, interpol_lanczos4, interpol_catmull_rom, interpol_centri_catmull_rom };

void bench_og(benchmark::State& s) {
    auto N = 8 << s.range(0);
    auto a = s.range(1);
    interpolador = inter[a];
    char* image = (char*)malloc(N * N * 3);

    // Main timing loop
    for (auto _ : s) {
        conversao_invUV(image + N * N, N / 2);
        conversao_invY(image, N);
    }

    s.SetBytesProcessed(uint64_t(s.iterations() * N * N * 2));
    s.SetItemsProcessed(s.iterations());
    s.SetLabel([](int size) {
        size_t dim = size * size * 2;
        if (dim < (1 << 10))
            return std::to_string(dim) + " pixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
        if (dim < (1 << 20))
            return std::to_string(dim >> 10) + " kpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
        if (dim < (1 << 30))
            return std::to_string(dim >> 20) + " Mpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
        return std::string("Empty");
    } (N));

    free(image);
}


#endif // !OG_H
