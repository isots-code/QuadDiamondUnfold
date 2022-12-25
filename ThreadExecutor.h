#pragma once
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>

class ThreadedExecutor {
public:

	explicit ThreadedExecutor(const std::size_t dim, const std::size_t numThreads = 1);
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
	const std::size_t dim;
	const std::size_t numThreads;
	std::atomic_bool mStopped;
	std::atomic_bool mRunning;
	std::vector<std::thread> mThreads;

	void start();

private:
	void loop(void);

protected:
	class DoubleBuffer {
	public:

		DoubleBuffer(void);

		void* write(void);
		const void* read(void);
		void swap(void);
		void setBuffers(void* curr, void* nxt);
		void signalStop(void);

	private:
		void* next = nullptr;
		void* current = nullptr;
		std::mutex stateMtx;
		bool stopExit = false;
		std::atomic_bool readValid;
		std::atomic_bool writeValid;
	};

	DoubleBuffer inputBuf;
	DoubleBuffer outputBuf;
};