#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>

#include <windows.h>
#include <process.h>

#include "frameData.h"
#include "frameDataCustom.h"
#include "AlignedVector.h"
#include "ffmpegDecode.h"

int main(int argc, char** argv) {

	using bitDepth = bits8_t;

	try {
		auto decoder = ffmpegDecode<bitDepth>(std::string(argv[1]));

		int dim = decoder.getDim();

		AlignedVector<bitDepth> out0(dim * dim * 2 * 3);
		AlignedVector<bitDepth> out1(dim * dim * 2 * 3);

		//auto dataLookup = frameDataCustom<bitDepth>(dim, 4, &centripetalCatMullRomInterpolation<bitDepth>);
		auto dataLookup = frameData<bitDepth>(dim, 4);
		dataLookup.setOBuffers(out0.data(), out1.data());

		decoder.connectFrameData(dataLookup);
		decoder.startDecode();

		std::thread([&]() {
			// Create pipes
			HANDLE hReadPipe, hWritePipe;
			SECURITY_ATTRIBUTES sa = {
				.lpSecurityDescriptor = nullptr,
				.bInheritHandle = TRUE
			};
			if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, dim * dim * 3 * 2)) {
				std::cout << "Failed to create pipes\n";
				return 1;
			}

			SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);

			STARTUPINFOA si = { 
				.dwFlags = STARTF_USESTDHANDLES,
				.hStdInput = hReadPipe,
				.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
				.hStdError = GetStdHandle(STD_ERROR_HANDLE)
			};

			PROCESS_INFORMATION pi = {};


			// Set up the command line for ffplay
			auto cmd = std::string("ffplay -f rawvideo -pixel_format yuv444p -video_size " + std::to_string(dim * 2) + "x" + std::to_string(dim) + " -i pipe:0");
			std::wstring commandLine(cmd.begin(), cmd.end());

			// Create the ffplay process
			
			if (!CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
				std::cout << "Failed to create process\n";
				return 1;
			}

			unsigned long bytesWritten;

			bool currentBuffer = 0;
			while (decoder.running) {
				dataLookup.readOutput();
				auto buffer = currentBuffer ? out0.data() : out1.data();
				if (!WriteFile(hWritePipe, buffer, dim * dim * 3 * 2, &bytesWritten, nullptr)) {
					std::cout << "Failed to write to pipe\n";
					return 1;
				}
				currentBuffer = !currentBuffer;
			}
			
			// Wait for the ffplay process to finish
			//WaitForSingleObject(processInfo.hProcess, INFINITE);

			// Clean up
			CloseHandle(hReadPipe);
			CloseHandle(hWritePipe);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			//while (decoder.running)
			//	dataLookup.readOutput();
		}).join();

	} catch (std::exception& e) {
		std::cerr << e.what();
		return -1;
	}

	return 0;
}