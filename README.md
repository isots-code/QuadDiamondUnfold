# QuadDiamondUnfold
 
# What's this?

This is a side project of mine to rework my bachelor's thesis final project and make it (someday) actually useful.

The initial idea was to make this work with images, video and possibily 360º video for demonstration. We got stuck on images.

The main purpose of this is to, in real time, do a sort of unfold of a video and make it viewable. Below is an example of the original image and the transformation. 

### Original
![Original](/media/unfolded.png "Original")

### Folded
![Folded](/media/folded.png "Folded")

This project is about reversing the transformation so as to be viewed as the original format.

Later, i will add code to do the transformation (but not a fast one) so you can try it on your videos.

This also is for me to get a bit more hands on with C++ and SIMD in x86 since my main language till a few months ago was embedded C.

Right now, it doesn't support processores earlier then Haswell cause it needs AVX2 instructions.

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