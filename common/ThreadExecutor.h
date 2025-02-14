#pragma once
#include <deque>
#include <queue>
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <barrier>
#include <functional>
#include <condition_variable>

class ThreadedExecutor {
public:

	explicit ThreadedExecutor(int inputSize, int outputSize, const std::size_t numThreads = 1);

	~ThreadedExecutor();

	const void* readOutput(void);
	void* writeInput(void);
	void setIOBuffers(void* inCurr, void* inNext, void* outCurr, void* outNext);
	void setIBuffers(void* inCurr, void* inNext);
	void setOBuffers(void* outCurr, void* outNext);

	void stop(void) noexcept;

	virtual void kernel(const int id) = 0;

protected:

	class DoubleBuffer;

	void start(void);
	void loop(void);
	void completionFunc(void) noexcept;

	void* input;
	void* output;
	const int numThreads;
	std::mutex mMutex;
	std::atomic_bool mFinished;
	std::condition_variable mCv;
	std::vector<std::thread> mThreads;

	std::unique_ptr<DoubleBuffer> readBuffer;
	std::unique_ptr<DoubleBuffer> writeBuffer;

	class DoubleBuffer {
	public:
		DoubleBuffer(void);
	
		void setBuffers(void* curr, void* next);
	
		void* Produce(void);
	
		void* Consume(void);
	
		void Stop(void);
	
	private:
		std::atomic_bool running;
		std::atomic_bool is_full;
		std::atomic_bool is_empty;
		void* active_buffer;
		void* inactive_buffer;
		std::mutex mutex;
		std::condition_variable full_cv;
		std::condition_variable empty_cv;
	};
	

};