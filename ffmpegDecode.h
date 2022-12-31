extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <thread>
#include <filesystem>
#include <mutex>
#include <condition_variable>
#include <queue>

struct frameData;

struct ffmpegDecode {
	ffmpegDecode(const std::filesystem::path& input, bool saving);
	ffmpegDecode(const std::filesystem::path& input);

	~ffmpegDecode();
	
	void connectFrameData(frameData& frameData);

	int getDim(void);

	bool getOp(void);

	void start(void);

	void decodeLoop(void);

	void saveLoop(void);

	void playLoop(void);

	volatile bool running;

private:
	AVFormatContext* format_ctx;
	AVCodecContext* codec_ctx;
	AVCodec* codec;
	AVFrame* frame;
	int video_stream_index;
	frameData* frameDataLookUp;

	volatile bool saving;
	volatile bool first;

	std::thread decodeThread;
	std::thread savingThread;
	std::thread playThread;
	std::mutex mutex;
	std::condition_variable cv;
	std::queue<void*> frame_buffer_queue;
};

