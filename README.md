# QuadDiamondUnfold
 
# What's this?

This is a side project of mine to rework my bachelor's thesis final project and make it (someday) actually useful.

The initial idea was to make this work with images, video and possibily 360º video for demonstration. We got stuck on images.

The main purpose of this is to, in real time, do a sort of unfold of a video and make it viewable.

Later, i will add code to do the transformation (but not a fast one) so you can try it on your videos.

This also is for me to get a bit more hands on with C++ and SIMD in x86 since my main language till a few months ago was embedded C.

This depends on Agner Fog's VCL library [Link: https://github.com/vectorclass/version2]

For testing, it depends on Google Benchmark [Link: https://github.com/google/benchmark]

The AlignedVector file comes from here on StackOverflow [link: https://stackoverflow.com/a/70994249]

If you come across this and saya its poorly organized, i'll take help in writing a better README, making a VCL plugin out of this and CMake.
For now, its just a bunch of source files in a git.

# Building

I built this on Visual Studio 2019 using Clang.

This ***should*** compile on Linux with both Clang and GCC. There might be a few things that won't, such as atributtes and pragmas, but not many, remove them at will.

If i did everything right, it should also produce next to no warnings.

Some of the args i used are std=c++20 or above (experimental in at least Clang 12), -Ofast (yes i know), and march=native