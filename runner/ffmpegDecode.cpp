#include "ffmpegDecode.h"

#include <string>
#include <utility>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <fstream>

#include <windows.h>
#include <process.h>

#include "frameData.h"

ffmpegDecode::ffmpegDecode(const std::filesystem::path& input, bool saving) : running(true), format_ctx(nullptr),
	codec_ctx(nullptr), codec(nullptr), video_stream_index(-1), frameDataLookUp(nullptr), saving(saving), first(true) {

	auto path = std::filesystem::canonical(input).string();
	// Open the input file
	if (avformat_open_input(&format_ctx, std::filesystem::canonical(input).string().c_str(), nullptr, nullptr) < 0)
		throw std::runtime_error(std::string("Error opening file: ") + input.filename().string().c_str() + "!");

	// Find the best video stream
	if (avformat_find_stream_info(format_ctx, nullptr) < 0)
		throw std::runtime_error("Error finding stream information!");

	// Find the video stream index
	for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
		if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = static_cast<int>(i);
			break;
		}
	}

	if (video_stream_index == -1)
		throw std::runtime_error("No video stream found!");

	codec_ctx = avcodec_alloc_context3(nullptr);
	if (!codec_ctx)
		throw std::runtime_error("Error allocating codec context");

	// Copy the codec parameters from the video stream
	if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[video_stream_index]->codecpar) < 0)
		throw std::runtime_error("Error copying codec parameters");

	// Find the codec
	codec = const_cast<AVCodec*>(avcodec_find_decoder(codec_ctx->codec_id));
	if (!codec)
		throw std::runtime_error("Codec not found");

	// Open the codec
	if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
		throw std::runtime_error("Error opening codec");

	frame = av_frame_alloc();
	if (!frame)
		throw std::runtime_error("Error allocating frame");

}

ffmpegDecode::ffmpegDecode(const std::filesystem::path& input) : ffmpegDecode(input, false) {}

ffmpegDecode::~ffmpegDecode() {
	decodeThread.join();
	playThread.join();
	if (saving) {
		{
			std::unique_lock<std::mutex> lock(mutex);
			cv.notify_all();
		}
		cv.notify_all();
		savingThread.join();
	}
	avcodec_close(codec_ctx);
	av_frame_free(&frame);
	avcodec_free_context(&codec_ctx);
	avformat_close_input(&format_ctx);
}

void ffmpegDecode::connectFrameData(frameData& frameData) {
	frameDataLookUp = &frameData;

	int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV444P, codec_ctx->width, codec_ctx->height, 1);

	switch (frameData.bitPerSubPixel) {
		case frameData::BITS_8:
			break;
		case frameData::BITS_9:
		case frameData::BITS_10:
		case frameData::BITS_11:
		case frameData::BITS_12:
		case frameData::BITS_13:
		case frameData::BITS_14:
		case frameData::BITS_15:
		case frameData::BITS_16:
			buffer_size *= 2;
			break;
	}

	buffers[0] = AlignedVector<uint8_t>(buffer_size);
	if (buffers[0].data() == nullptr)
		throw std::runtime_error("Error allocating buffer");

	buffers[1] = AlignedVector<uint8_t>(buffer_size);
	if (buffers[1].data() == nullptr)
		throw std::runtime_error("Error allocating buffer");

	frameDataLookUp->setIBuffers(buffers[0].data(), buffers[1].data());
}

int ffmpegDecode::getDim(void) { return codec_ctx->height; }

bool ffmpegDecode::getOp(void) { return !(codec_ctx->height == codec_ctx->width); }

void ffmpegDecode::start(void) {
	playThread = std::thread(&ffmpegDecode::playLoop, this);
	decodeThread = std::thread(&ffmpegDecode::decodeLoop, this);
	if (saving)
		savingThread = std::thread(&ffmpegDecode::saveLoop, this);
}

void ffmpegDecode::decodeLoop(void) {

	bool currentBuffer = false;
	AVPacket packet;
	while (av_read_frame(format_ctx, &packet) >= 0) {
	  // Check if the packet is from the video stream
		if (packet.stream_index == video_stream_index) {
		  // Decode the video frame
			int ret = avcodec_send_packet(codec_ctx, &packet);
			if (ret < 0) {
			  // Error sending packet
				break;
			}

			ret = avcodec_receive_frame(codec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			  // No more frames to decode
				continue;
			} else if (ret < 0) {
			  // Error decoding frame
				break;
			}

			// Frame successfully decoded
			// Fill the array with the raw frame data
			switch (codec_ctx->pix_fmt) {

				case AV_PIX_FMT_YUVJ444P:
				case AV_PIX_FMT_YUV444P:
					ret = av_image_copy_to_buffer(reinterpret_cast<uint8_t*>(buffers[currentBuffer].data()), codec_ctx->width * codec_ctx->height * 3, frame->data, frame->linesize, AV_PIX_FMT_YUV444P, codec_ctx->width, codec_ctx->height, 1);
					break;

				case AV_PIX_FMT_YUVJ420P:
				case AV_PIX_FMT_YUV420P:
					ret = av_image_copy_to_buffer(reinterpret_cast<uint8_t*>(buffers[currentBuffer].data()), codec_ctx->width * codec_ctx->height * 3 / 2, frame->data, frame->linesize, AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height, 1);
					frameData::expandUV(buffers[currentBuffer].data() + codec_ctx->width * codec_ctx->height, codec_ctx->width, codec_ctx->height);
					break;
				default:
					throw std::runtime_error("Unsuported pixel format");

			}

			if (ret < 0)
				throw std::runtime_error("Error filling array");

			frameDataLookUp->writeInput();
			currentBuffer = !currentBuffer;

			if (first) [[unlikely]]
				first = false;

		}
		// Free the packet
		av_packet_unref(&packet);

	}
	running = false;
	frameDataLookUp->writeInput();
}

void ffmpegDecode::saveLoop(void) {

	while (first);

	int dim = codec_ctx->height;
	bool op = getOp();
	// Create pipes
	HANDLE hReadPipe, hWritePipe;
	SECURITY_ATTRIBUTES sa = {
		.lpSecurityDescriptor = nullptr,
		.bInheritHandle = TRUE
	};
	if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, static_cast<unsigned int>((dim * dim * 3) * (op ? 2 : 1)))) {
		std::cout << "Failed to create pipes";
		return;
	}

	SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = {
		.dwFlags = STARTF_USESTDHANDLES,
		.hStdInput = hReadPipe,
		.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
		.hStdError = GetStdHandle(STD_ERROR_HANDLE),
	};

	PROCESS_INFORMATION pi = {};

	// Set up the command line for ffplay
	std::string cmd("ffmpeg -f rawvideo -pixel_format yuvj444p -r ");
	cmd += std::to_string(codec_ctx->framerate.num) + "/" + std::to_string(codec_ctx->framerate.den);
	cmd += " -video_size ";
	if (op)
		cmd += std::to_string(dim) + "x" + std::to_string(dim) + " -i pipe:0";
	else
		cmd += std::to_string(dim * 2) + "x" + std::to_string(dim) + " -i pipe:0";
	cmd += " -pixel_format yuvj444p -c:v libx264 -preset ultrafast -b:v 10M -y output_" + std::string(op ? "compressed" : "decompressed") + ".mp4";
	//cmd += " -pixel_format yuvj444p -c:v libx264 -preset placebo -crf 0 -y output_" + std::string(op ? "compressed" : "decompressed") + ".mp4";

	// Create the ffplay process
	if (!CreateProcessA(nullptr, const_cast<const LPSTR>(cmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
		std::cout << "Failed to create process";
		return;
	}

	//std::string filename("comp_2048x2048_yuv444.yuv");
	//std::ofstream outFile(filename, std::ios::binary);
	//if (!outFile) {
	//	std::cerr << "Error opening file for writing: " << filename << std::endl;
	//	return;
	//}

	unsigned long bytesWritten;
	while (running) {
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock);
		if (!running)
			break;
		if (frame_buffer_queue.empty())
			continue;
		auto buffer = frame_buffer_queue.front();
		frame_buffer_queue.pop();
		if (!WriteFile(hWritePipe, buffer, static_cast<DWORD>((dim * dim * 3) * (op ? 1 : 2)), &bytesWritten, nullptr)) {
			std::cout << "Failed to write to pipe";
			break;
		}

		//outFile.write(reinterpret_cast<char*>(buffer), dim * dim * 3);
		std::free(buffer);
	}
	//outFile.close();

	// Clean up
	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void ffmpegDecode::playLoop(void) {

	while (first);

	int dim = codec_ctx->height;
	bool op = getOp();

	// Create pipes
	HANDLE hReadPipe, hWritePipe;
	SECURITY_ATTRIBUTES sa = {
		.lpSecurityDescriptor = nullptr,
		.bInheritHandle = TRUE
	};
	if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, static_cast<DWORD>((dim * dim * 3) * (op ? 2 : 1)))) {
		std::cout << "Failed to create pipes\n";
		return;
	}

	SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = {
		.dwFlags = STARTF_USESTDHANDLES,
		.hStdInput = hReadPipe,
		.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
		.hStdError = GetStdHandle(STD_ERROR_HANDLE),
	};

	PROCESS_INFORMATION pi = {};

	// Set up the command line for ffplay
	std::string cmd("ffplay -autoexit ");
	if (op)
		cmd += "-x 720 -y 720";
	else
		cmd += "-x 1440 -y 720";
	cmd += " -f rawvideo -pixel_format yuv444p -framerate ";
	cmd += std::to_string(codec_ctx->framerate.num) + "/" + std::to_string(codec_ctx->framerate.den);
	cmd += " -video_size ";
	if (op)
		cmd += std::to_string(dim) + "x" + std::to_string(dim) + " -i pipe:0";
	else
		cmd += std::to_string(dim * 2) + "x" + std::to_string(dim) + " -i pipe:0";

	// Create the ffplay process
	if (!CreateProcessA(nullptr, const_cast<const LPSTR>(cmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
		std::cout << "Failed to create process\n";
		return;
	}

	unsigned long bytesWritten;
	while (running) {
		auto buffer = frameDataLookUp->readOutput();
		if (!WriteFile(hWritePipe, buffer, static_cast<DWORD>((dim * dim * 3) * (op ? 1 : 2)), &bytesWritten, nullptr)) {
			std::cout << "Failed to write to pipe";
			break;
		}
		if (saving) {
			auto bufferCopy = std::malloc(static_cast<size_t>((dim * dim * 3) * (op ? 1 : 2)));
			std::memcpy(bufferCopy, buffer, static_cast<size_t>((dim * dim * 3) * (op ? 1 : 2)));
			std::unique_lock<std::mutex> lock(mutex);
			frame_buffer_queue.push(bufferCopy);
			cv.notify_one();
		}
	}

	// Clean up
	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}
