# QuadDiamondUnfold

# CHANGES

<details>

# NEW (30-11-23)

Added some benchmarks for both my PC's for comparison and to show results plus some other small changes. Check commit history for details.

# NEW (28-12-22)

Compressor code has been added, it auto detects if both dimensions are the same and select the appropriate operation. It even has a (mostly) vectorized function.

Also new, dynamic dispatch, compile all code with normal arguments and the files ending in AVX2 and link it all together, it will dispath according to your CPU.

# NEW (26-12-22)

Scalar code has been added, so its now possible to run on stuff older then Haswell, although, not recommended as the performance is weak.
For comparison, my R7 5700x can do around 250 FPS on a 4k \* 2k frame size, my old i7 4720HQ can only do 55 IIRC.

</details>
 
# What's this?

This is a side project of mine to rework my bachelor's thesis final project and make it (someday) actually useful.

The initial idea was to make this work with images, video and possibily 360º video for demonstration. We got stuck on images.

The main purpose of this is to, in real time, do a sort of unfold of a video and make it viewable. Below is an example of the original image and the transformation. 

# Example Images

<details>

### Original
![Original](/media/unfolded.png "Original")

### Folded
![Folded](/media/folded.png "Folded")

</details>

# Example Videos
<details>

### Input video
https://github.com/isots-code/QuadDiamondUnfold/assets/23300310/fcb36462-322d-4be5-b8a1-257ca26f6cea

### Compressed Output
https://github.com/user-attachments/assets/c875d080-d412-466a-b99f-bcc4036b6231

### Decompressed Output
https://github.com/user-attachments/assets/6f6034b9-aa96-4ccd-a5a3-e84d7151fb7d

</details>

# More info

This project is about reversing the transformation so as to be viewed as the original format.

~~Later, i will add code to do the transformation (but not a fast one) so you can try it on your videos.~~ Done

This also is for me to get a bit more hands on with C++ and SIMD in x86 since my main language till a few months ago was embedded C.

~~Right now, it doesn't support processores earlier then Haswell cause it needs AVX2 instructions.~~

This depends on [Agner Fog's VCL library](https://github.com/vectorclass/version2)

For testing performance, it depends on [Google Benchmark](https://github.com/google/benchmark)

The AlignedVector file comes from [here on StackOverflow](https://stackoverflow.com/a/70994249)

Centripetal Catmull-Rom interpolation code obtained [from here](https://qroph.github.io/2018/07/30/smooth-paths-using-catmull-rom-splines.html)

For video decoding and display i use [FFMPEG](https://ffmpeg.org/)

Lanczos interpolation code reverse-engineered from [OpenCV](https://github.com/opencv/opencv/blob/4abe6dc48d4ec6229f332cc6cf6c7e234ac8027e/modules/imgproc/src/resize.cpp#L918)

If you come across this and say it's poorly organized, i'll gladly take help in writing a better README, making a VLC plugin out of this and how to use a build system for this.
For now, its just a bunch of source files in a git.

# Building

I built this on Visual Studio 2019 using Clang.

This ***should*** compile on Linux with both Clang and GCC. There might be a few things that won't, such as atributtes and pragmas, but not many, remove them at will.

If i did everything right, it should also produce next to no warnings.

Some of the args i used are std=c++20 or above (experimental in at least Clang 12), -Ofast (yes i know), and march=native

# Benchmarks

I've ran some benchmarks comparing the original version of the code that was developed to the improved version.

In a lot of cases, an improvement of 50x is achieved, sometimes 100x, and we have become memory bandwidth limited for larger input sizes.

The results can be view here:

[i7-4720HQ](/benchmarks/i7-4720hq_ddr3_1600MTs.txt)

[R7-5700X](/benchmarks/r7_5700x_ddr4_3600MTs_tuned.txt)

Here's a small snippet on my desktop 5700X machine, the rest of the numbers and the run on the 4720HQ can be found on the [benchmarks](/benchmarks) folder.

<details>
	<summary>Benchmark Summary</summary>

```
2023-11-29T12:16:02+00:00
Running QuadDiamondUnfold.exe
Run on (16 X 3400 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 32768 KiB (x1)
---------------------------------------------------------------------------------------------------------------------------------------
Benchmark                                            Wall Time		UserCounters...
---------------------------------------------------------------------------------------------------------------------------------------

512 kipixels (1024 * 512)
bench_og/6/nearest                                     14.1 ms		53.0350 Mpixels/s items_per_second=70.7133 FPS
bench_og/6/linear                                      7.40 ms		101.348 Mpixels/s items_per_second=135.130 FPS
bench_og/6/cubic                                       9.65 ms		77.7073 Mpixels/s items_per_second=103.610 FPS
bench_og/6/lanczos2                                    21.2 ms		35.4120 Mpixels/s items_per_second=47.2160 FPS
bench_og/6/lanczos3                                    26.0 ms		28.8687 Mpixels/s items_per_second=38.4916 FPS
bench_og/6/lanczos4                                    27.5 ms		27.2591 Mpixels/s items_per_second=36.3454 FPS
bench_og/6/catmull_rom                                 9.76 ms		76.8693 Mpixels/s items_per_second=102.492 FPS
bench_og/6/centri_catmull_rom                          20.3 ms		37.0110 Mpixels/s items_per_second=49.3480 FPS
bench_avx_multi_nearest/16/6/1                        0.181 ms		2.70492 Gpixels/s items_per_second=5539.68 FPS
bench_avx_multi_linear/16/6/1                         0.189 ms		2.58659 Gpixels/s items_per_second=5297.34 FPS
bench_avx_multi_cubic/16/6/1                          0.211 ms		2.31328 Gpixels/s items_per_second=4737.60 FPS
bench_avx_multi_catmull_rom/16/6/1                    0.211 ms		2.31625 Gpixels/s items_per_second=4743.67 FPS
bench_avx_multi_lanczos2/16/6/1                       0.214 ms		2.28239 Gpixels/s items_per_second=4674.33 FPS
bench_avx_multi_lanczos3/16/6/1                       0.238 ms		2.04911 Gpixels/s items_per_second=4196.58 FPS
bench_avx_multi_lanczos4/16/6/1                       0.259 ms		1.88574 Gpixels/s items_per_second=3862.00 FPS
bench_avx_multi_lanczosN/16/6/2                       0.191 ms		2.56039 Gpixels/s items_per_second=5243.69 FPS
bench_avx_multi_lanczosN/16/6/4                       0.213 ms		2.29768 Gpixels/s items_per_second=4705.65 FPS
bench_avx_multi_lanczosN/16/6/8                       0.260 ms		1.88035 Gpixels/s items_per_second=3850.95 FPS
bench_avx_multi_lanczosN/16/6/16                      0.354 ms		1.38033 Gpixels/s items_per_second=2826.92 FPS
bench_avx_multi_centrip_catmull_rom/16/6/1            0.545 ms		918.356 Mpixels/s items_per_second=1836.71 FPS
																	
8 Mipixels (4096 * 2048)                                       		
bench_og/8/nearest                                      228 ms		52.5799 Mpixels/s items_per_second=4.38166 FPS
bench_og/8/linear                                       120 ms		99.6724 Mpixels/s items_per_second=8.30603 FPS
bench_og/8/cubic                                        155 ms		77.3961 Mpixels/s items_per_second=6.44967 FPS
bench_og/8/lanczos2                                     338 ms		35.5543 Mpixels/s items_per_second=2.96286 FPS
bench_og/8/lanczos3                                     416 ms		28.8405 Mpixels/s items_per_second=2.40337 FPS
bench_og/8/lanczos4                                     441 ms		27.2017 Mpixels/s items_per_second=2.26681 FPS
bench_og/8/catmull_rom                                  158 ms		75.9009 Mpixels/s items_per_second=6.32507 FPS
bench_og/8/centri_catmull_rom                           325 ms		36.8868 Mpixels/s items_per_second=3.07390 FPS
bench_avx_multi_nearest/16/8/1                         3.26 ms		2.39512 Gpixels/s items_per_second=306.576 FPS
bench_avx_multi_linear/16/8/1                          3.43 ms		2.27801 Gpixels/s items_per_second=291.586 FPS
bench_avx_multi_cubic/16/8/1                           3.91 ms		1.99581 Gpixels/s items_per_second=255.463 FPS
bench_avx_multi_catmull_rom/16/8/1                     3.89 ms		2.00864 Gpixels/s items_per_second=257.107 FPS
bench_avx_multi_lanczos2/16/8/1                        3.85 ms		2.02669 Gpixels/s items_per_second=259.417 FPS
bench_avx_multi_lanczos3/16/8/1                        4.35 ms		1.79421 Gpixels/s items_per_second=229.659 FPS
bench_avx_multi_lanczos4/16/8/1                        4.83 ms		1.61641 Gpixels/s items_per_second=206.901 FPS
bench_avx_multi_lanczosN/16/8/2                        3.42 ms		2.28759 Gpixels/s items_per_second=292.811 FPS
bench_avx_multi_lanczosN/16/8/4                        3.88 ms		2.01108 Gpixels/s items_per_second=257.418 FPS
bench_avx_multi_lanczosN/16/8/8                        4.87 ms		1.60507 Gpixels/s items_per_second=205.449 FPS
bench_avx_multi_lanczosN/16/8/16                       7.50 ms		1.04143 Gpixels/s items_per_second=133.303 FPS
bench_avx_multi_centrip_catmull_rom/16/8/1             8.49 ms		942.843 Mpixels/s items_per_second=117.855 FPS
																	
128 Mipixels (16384 * 8192)                                    		
bench_og/10/nearest                                    3706 ms		51.8092 Mpixels/s items_per_second=0.26983 FPS 
bench_og/10/linear                                     1988 ms		96.5612 Mpixels/s items_per_second=0.50292 FPS
bench_og/10/cubic                                      2542 ms		75.5305 Mpixels/s items_per_second=0.39338 FPS
bench_og/10/lanczos2                                   5461 ms		35.1566 Mpixels/s items_per_second=0.18310 FPS
bench_og/10/lanczos3                                   6714 ms		28.5956 Mpixels/s items_per_second=0.14893 FPS
bench_og/10/lanczos4                                   7115 ms		26.9872 Mpixels/s items_per_second=0.14055 FPS
bench_og/10/catmull_rom                                2588 ms		74.1748 Mpixels/s items_per_second=0.38632 FPS
bench_og/10/centri_catmull_rom                         5270 ms		36.4347 Mpixels/s items_per_second=0.18976 FPS
bench_avx_multi_nearest/16/10/1                        76.1 ms		1.64422 Gpixels/s items_per_second=13.1537 FPS
bench_avx_multi_linear/16/10/1                         77.1 ms		1.62248 Gpixels/s items_per_second=12.9798 FPS
bench_avx_multi_cubic/16/10/1                          84.7 ms		1.47615 Gpixels/s items_per_second=11.8092 FPS
bench_avx_multi_catmull_rom/16/10/1                    85.3 ms		1.46635 Gpixels/s items_per_second=11.7308 FPS
bench_avx_multi_lanczos2/16/10/1                       85.5 ms		1.46264 Gpixels/s items_per_second=11.7011 FPS
bench_avx_multi_lanczos3/16/10/1                       93.0 ms		1.34438 Gpixels/s items_per_second=10.7550 FPS
bench_avx_multi_lanczos4/16/10/1                        103 ms		1.20938 Gpixels/s items_per_second=9.67502 FPS
bench_avx_multi_lanczosN/16/10/2                       76.7 ms		1.62998 Gpixels/s items_per_second=13.0398 FPS
bench_avx_multi_lanczosN/16/10/4                       85.2 ms		1.46730 Gpixels/s items_per_second=11.7384 FPS
bench_avx_multi_lanczosN/16/10/8                        103 ms		1.21449 Gpixels/s items_per_second=9.71593 FPS
bench_avx_multi_lanczosN/16/10/16                       141 ms		909.507 Mpixels/s items_per_second=7.10552 FPS
bench_avx_multi_centrip_catmull_rom/16/10/1             146 ms		874.037 Mpixels/s items_per_second=6.82842 FPS
```
</details>
