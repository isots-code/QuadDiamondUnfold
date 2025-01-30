#pragma once
#include <deque>
#include <queue>
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <barrier>
#include <condition_variable>

class ThreadedExecutor {
public:

	explicit ThreadedExecutor(int inputSize, int outputSize, const std::size_t numThreads = 1);

	~ThreadedExecutor();

	void* getInputBuffer(void);
	void queueInputBuffer(void* buffer);

	void returnOutputBuffer(void* buffer);
	void* dequeueOutputBuffer(void);

	void stop(void) noexcept;

	virtual void kernel(const int id) = 0;

protected:


	void start(void);
	void loop(void);

	void* input;
	void* output;
	const int numThreads;
	std::mutex mMutex;
	std::atomic_bool mFinished;
	std::condition_variable mCv;
	std::vector<std::thread> mThreads;

	//class DoubleBuffer {
	//public:
	//	DoubleBuffer(void);
	//
	//	void setBuffers(void* curr, void* next);
	//
	//	void* Produce(void);
	//
	//	void* Consume(void);
	//
	//	void Stop(void);
	//
	//private:
	//	std::atomic_bool running;
	//	std::atomic_bool is_full;
	//	std::atomic_bool is_empty;
	//	void* active_buffer;
	//	void* inactive_buffer;
	//	std::mutex mutex;
	//	std::condition_variable full_cv;
	//	std::condition_variable empty_cv;
	//};
	//
	//DoubleBuffer readBuffer;
	//DoubleBuffer writeBuffer;

	class Queue {
	public:
		Queue(void);
		void Stop(void);

		void enqueue(void* buffer);
		void* dequeue();

	private:
		bool mStopping;
		std::mutex mMutex;
		std::queue<void*> mQueue;
		std::condition_variable mCvDeq;
		std::condition_variable mCvEnq;
	};

	Queue inputQueue;
	Queue outputQueue;

	class BufferPool {
	public:
		BufferPool(int bufferSize);
		~BufferPool();

		void* GetBuffer();
		void ReturnBuffer(void* buffer);

	private:
		int mBufferSize; // Size of each buffer in the pool
		std::mutex mMutex; // Mutex to protect access to the pool
		std::vector<void*> mBuffers; // All of the buffers in the pool
		std::deque<void*> mAvailableBuffers; // Buffers that are currently available for use
	};

	BufferPool inputBufferPool;
	BufferPool outputBufferPool;

};