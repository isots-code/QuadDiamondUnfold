extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <thread>
#include <filesystem>

#include "AlignedVector.h"

struct frameData;

struct ffmpegDecode {
	ffmpegDecode(const std::filesystem::path& input);

	~ffmpegDecode();
	
	void connectFrameData(frameData& frameData);

	int getDim(void);

	bool getOp(void);

	void startDecode(void);

	void decodeLoop(void);

	void startFFPlay(bool op);

	bool running;
	AlignedVector<uint8_t> buffers[2];

private:
	AVFormatContext* format_ctx;
	AVCodecContext* codec_ctx;
	AVCodec* codec;
	AVFrame* frame;
	int video_stream_index;
	frameData* frameDataLookUp;

	std::thread decodeThread;
};

