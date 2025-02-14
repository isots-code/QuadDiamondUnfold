#include <iostream>
#include <cstddef>
#include <chrono>

#include "ThreadExecutor.h"

template <class T>
inline auto doNotOptimizeAway(T const& datum) {
	return reinterpret_cast<char const volatile&>(datum);
}

ThreadedExecutor::ThreadedExecutor(int inputSize, int outputSize, const std::size_t numThreads)
	: input(nullptr), output(nullptr), numThreads(numThreads), mFinished(false), 
	readBuffer(std::make_unique<DoubleBuffer>()), writeBuffer(std::make_unique<DoubleBuffer>()) {
	(void)inputSize;
	(void)outputSize;
	start();
}

ThreadedExecutor::~ThreadedExecutor() {}

const void* ThreadedExecutor::readOutput(void) { return writeBuffer->Consume(); }

void* ThreadedExecutor::writeInput(void) { return  readBuffer->Produce(); }

void ThreadedExecutor::setIOBuffers(void* inCurr, void* inNext, void* outCurr, void* outNext) {
	setIBuffers(inCurr, inNext);
	setOBuffers(outCurr, outNext);
}

void ThreadedExecutor::setIBuffers(void* inCurr, void* inNext) {
	readBuffer->setBuffers(inCurr, inNext);
}

void ThreadedExecutor::setOBuffers(void* outCurr, void* outNext) {
	writeBuffer->setBuffers(outCurr, outNext);
}

void ThreadedExecutor::stop(void) noexcept {
	{
		std::unique_lock<std::mutex> lock(mMutex);
		mFinished = true;
	}
	readBuffer->Stop();
	writeBuffer->Stop();
	for (auto& thread : mThreads)
		thread.join();
	mCv.notify_all();
}

void ThreadedExecutor::start(void) { std::thread(&ThreadedExecutor::loop, this).detach(); }

void ThreadedExecutor::loop(void) {

	std::barrier barrier(numThreads, [this]() noexcept { completionFunc(); });

	for (auto i = 0; i < numThreads; ++i) {
		mThreads.emplace_back([this, &barrier](int i) {
			while (true) {
				barrier.arrive_and_wait();
				if (mFinished) [[unlikely]] break; //exit early if we where told to stop
				kernel(i);
			}
			barrier.arrive_and_drop(); //we have to resync
		}, i);
	}

	{
		std::unique_lock<std::mutex> lock(mMutex);
		mCv.wait(lock, [this] { return mFinished.load(); });
	}

}

void ThreadedExecutor::completionFunc() noexcept {
		//using namespace std::chrono;
		//auto execTime = high_resolution_clock::now(); // we check time b4 we wait for buffers as we just finished the kernel
		////doNotOptimizeAway(execTime);
	if (!mFinished) [[likely]] input = readBuffer->Consume();
	if (!mFinished) [[likely]] output = writeBuffer->Produce();
	if (mFinished) [[unlikely]] return;
	//static bool startup = true;
	//static auto frames = 0ull;
	//static auto lastTime = high_resolution_clock::now();
	//if (!startup) [[likely]] {
	//	auto duration = execTime - lastTime;
	//	//doNotOptimizeAway(duration);
	//	static auto durationTotal = static_cast<decltype(duration)>(0);
	//	static auto durationTotalOverhead = static_cast<decltype(duration)>(0);
	//	static auto lastPrintTime = lastTime;
	//	durationTotal += duration;
	//	lastTime = high_resolution_clock::now();
	//	//doNotOptimizeAway(temp);
	//	durationTotalOverhead += lastTime - execTime;
	//	frames++;
	//	if (duration_cast<milliseconds>(lastTime - lastPrintTime).count() > 125) {
	//		//std::printf("\rfps: %10.3f avg: %10.3f frames done:%10llu overhead:%7.3f %%\t",
	//		//	1e9 / duration.count(),
	//		//	1e9 * frames / durationTotal.count(),
	//		//	frames,
	//		//	(durationTotalOverhead.count() * 100.0) / (durationTotal.count() + durationTotalOverhead.count())
	//		//);
	//		lastPrintTime = lastTime;
	//	}
	//}
	//startup = false;
}

ThreadedExecutor::DoubleBuffer::DoubleBuffer(void) : running(true), is_full(false), is_empty(true) {}

void ThreadedExecutor::DoubleBuffer::setBuffers(void* curr, void* next) {
	active_buffer = curr;
	inactive_buffer = next;
}

void* ThreadedExecutor::DoubleBuffer::Produce(void) {
	std::unique_lock<std::mutex> lock(mutex);

	full_cv.wait(lock, [this] { return !is_full || !running.load(); });

	if (!running) return nullptr;

	// Producer writes into inactive buffer
	is_full = true;
	is_empty = false;

	// Notify consumer that new data is available
	empty_cv.notify_one();

	return inactive_buffer; // Producer always writes into inactive buffer
}

void* ThreadedExecutor::DoubleBuffer::Consume(void) {
	std::unique_lock<std::mutex> lock(mutex);

	empty_cv.wait(lock, [this] { return !is_empty || !running.load(); });

	if (!running) return nullptr;

	// Swap buffers so consumer reads from the last produced data
	std::swap(active_buffer, inactive_buffer);

	is_full = false;
	is_empty = true;

	// Notify producer that buffer space is available
	full_cv.notify_one();

	return active_buffer; // Consumer reads from active buffer
}

void ThreadedExecutor::DoubleBuffer::Stop(void) {
	std::unique_lock<std::mutex> lock(mutex);
	running = false;
	is_full = true;
	empty_cv.notify_all();
	full_cv.notify_all();
}
