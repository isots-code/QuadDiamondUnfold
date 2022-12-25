extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <thread>
#include <utility>
#include <exception>
#include <stdexcept>
#include <filesystem>

#include "frameData.h"
#include "AlignedVector.h"

template <typename T>
struct ffmpegDecode {
	ffmpegDecode(const std::filesystem::path& input) : running(true), format_ctx(nullptr),
		codec_ctx(nullptr), codec(nullptr), video_stream_index(-1), frameData(nullptr) {

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
				video_stream_index = i;
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

	~ffmpegDecode() {
		decodeThread.join();
		avcodec_close(codec_ctx);
		av_frame_free(&frame);
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&format_ctx);
	}
	
	void connectFrameData(frameData<T>& frameData) {
		this->frameData = &frameData;

		int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV444P, codec_ctx->width, codec_ctx->height, 1);

		buffers[0] = std::move(AlignedVector<T>(buffer_size));
		if (buffers[0].data() == nullptr)
			throw std::runtime_error("Error allocating buffer");

		buffers[1] = std::move(AlignedVector<T>(buffer_size));
		if (buffers[1].data() == nullptr)
			throw std::runtime_error("Error allocating buffer");

		frameData.setIBuffers(buffers[0].data(), buffers[1].data());

	}

	int getDim(void) {
		return codec_ctx->height;
	}

	void startDecode(void) {
		decodeThread = std::move(std::thread(&ffmpegDecode::decodeLoop, this));
	}

	void decodeLoop(void) {

		bool currentBuffer;
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
				ret = av_image_copy_to_buffer(reinterpret_cast<uint8_t*>(buffers[currentBuffer].data()), codec_ctx->width * codec_ctx->height * 3, frame->data, frame->linesize, AV_PIX_FMT_YUV444P, codec_ctx->width, codec_ctx->height, 1);
				if (ret < 0)
					throw std::runtime_error("Error filling array");

				frameData->writeInput();
				currentBuffer = !currentBuffer;
				
			}
			// Free the packet
			av_packet_unref(&packet);

		}
		running = false;
		//frameData->writeInput();
	}

	bool running;
	AlignedVector<T> buffers[2];

private:
	AVFormatContext* format_ctx;
	AVCodecContext* codec_ctx;
	AVCodec* codec;
	AVFrame* frame;
	int video_stream_index;
	frameData<T>* frameData;

	std::thread decodeThread;
};

