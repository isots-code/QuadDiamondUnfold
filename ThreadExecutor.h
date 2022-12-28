#pragma once
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>

class ThreadedExecutor {
public:

	explicit ThreadedExecutor(const std::size_t numThreads = 1);

	~ThreadedExecutor();

	const void* readOutput(void);
	void* writeInput(void);
	void setIOBuffers(void* inCurr, void* inNext, void* outCurr, void* outNext);
	void setIBuffers(void* inCurr, void* inNext);
	void setOBuffers(void* outCurr, void* outNext);
	void stop(void) noexcept;

	virtual void kernel(const int id) = 0;

protected:

	void* output;
	const void* input;
	const std::size_t numThreads;
	std::mutex mutex;
	std::atomic_bool finished;
	std::condition_variable cv;
	std::vector<std::thread> mThreads;

	void completionFunc(void);

	void start();

	struct Barrier {
		std::size_t size;
		mutable std::mutex m;
		std::condition_variable cv;
		std::ptrdiff_t remaining;
		std::ptrdiff_t phase = 0;
		ThreadedExecutor* parent;

		Barrier(std::size_t s, ThreadedExecutor& parent);

		void arrive_and_wait();

		void arrive_and_drop();

	};

	Barrier barrier;

	class DoubleBuffer {
	public:
		DoubleBuffer(void);

		void setBuffers(void* curr, void* next);

		void* Produce(void);

		void* Consume(void);

		void Stop(void);

	private:
		bool running;
		volatile bool is_full;
		volatile bool is_empty;
		void* active_buffer;
		void* inactive_buffer;
		std::mutex mutex;
		std::condition_variable full_cv;
		std::condition_variable empty_cv;
	};

	DoubleBuffer readBuffer;
	DoubleBuffer writeBuffer;

};