#include <iostream>
#include <cstddef>
#include <chrono>

#include "ThreadExecutor.h"

template <class T>
inline auto doNotOptimizeAway(T const& datum) {
	return reinterpret_cast<char const volatile&>(datum);
}


ThreadedExecutor::ThreadedExecutor(const std::size_t numThreads)
	: numThreads(numThreads), finished(false), barrier(numThreads, *this) {
	start();
}

ThreadedExecutor::~ThreadedExecutor() { stop(); }

const void* ThreadedExecutor::readOutput(void) { return writeBuffer.Consume(); }

void* ThreadedExecutor::writeInput(void) { return  readBuffer.Produce(); }

void ThreadedExecutor::setIOBuffers(void* inCurr, void* inNext, void* outCurr, void* outNext) {
	setIBuffers(inCurr, inNext);
	setOBuffers(outCurr, outNext);
}

void ThreadedExecutor::setIBuffers(void* inCurr, void* inNext) {
	readBuffer.setBuffers(inCurr, inNext);
}

void ThreadedExecutor::setOBuffers(void* outCurr, void* outNext) {
	writeBuffer.setBuffers(outCurr, outNext);
}

void ThreadedExecutor::stop(void) noexcept {
	if (finished) return;
	{
		std::unique_lock lock(mutex);
		finished = true;
	}
	cv.notify_all();
	readBuffer.Stop();
	writeBuffer.Stop();
	for (auto& thread : mThreads)
		thread.join();
}

void ThreadedExecutor::completionFunc(void) {
	using namespace std::chrono;
	auto execTime = high_resolution_clock::now(); // we check time b4 we wait for buffers as we just finished the kernel
	//doNotOptimizeAway(execTime);
	if (!finished) [[likely]] input = readBuffer.Consume();
	if (!finished) [[likely]] output = writeBuffer.Produce();
	if (finished) [[unlikely]] return;
	static bool startup = true;
	static auto frames = 0ull;
	static auto lastTime = high_resolution_clock::now();
	if (!startup) [[likely]] {
		auto duration = execTime - lastTime;
		//doNotOptimizeAway(duration);
		static auto durationTotal = static_cast<decltype(duration)>(0);
		static auto durationTotalOverhead = static_cast<decltype(duration)>(0);
		static auto lastPrintTime = lastTime;
		durationTotal += duration;
		lastTime = high_resolution_clock::now();
		//doNotOptimizeAway(temp);
		durationTotalOverhead += lastTime - execTime;
		frames++;
		if (duration_cast<milliseconds>(lastTime - lastPrintTime).count() > 125) {
			//std::printf("\rfps: %10.3f avg: %10.3f frames done:%10llu overhead:%7.3f %%\t",
			//	1e9 / duration.count(),
			//	1e9 * frames / durationTotal.count(),
			//	frames,
			//	(durationTotalOverhead.count() * 100.0) / (durationTotal.count() + durationTotalOverhead.count())
			//);
			lastPrintTime = lastTime;
		}
	}
	startup = false;
};

void ThreadedExecutor::start() {
	for (auto i = 0u; i < numThreads; ++i) {
		mThreads.emplace_back([this](int i) {
			while (true) {
				barrier.arrive_and_wait();
				if (finished) [[unlikely]] break; //exit early if we where told to stop
				kernel(i);
			}
			barrier.arrive_and_drop(); //we have to resync
		}, i);
	}
}

ThreadedExecutor::Barrier::Barrier(std::size_t s, ThreadedExecutor& parent) : size(s), remaining(s), parent(&parent) {}

void ThreadedExecutor::Barrier::arrive_and_wait() {
	auto l = std::unique_lock(m);
	--remaining;
	if (remaining != 0) {
		auto myphase = phase + 1;
		cv.wait(l, [&] {
			return myphase - phase <= 0;
		});
	} else {
		parent->completionFunc();
		remaining = size;
		++phase;
		cv.notify_all();
	}
}
void ThreadedExecutor::Barrier::arrive_and_drop() {
	auto l = std::unique_lock(m);
	--size;
	--remaining;
	if (remaining == 0) {
		parent->completionFunc();
		remaining = size;
		++phase;
		cv.notify_all();
	}
}

ThreadedExecutor::DoubleBuffer::DoubleBuffer(void) : running(true), is_full(false), is_empty(true) {}

void ThreadedExecutor::DoubleBuffer::setBuffers(void* curr, void* next) {
	active_buffer = curr;
	inactive_buffer = next;
}

void* ThreadedExecutor::DoubleBuffer::Produce(void) {
	std::unique_lock lock(mutex);
	while (is_full) {
		if (!running) return nullptr;
		// Wait for the consumer to consume some data
		// and signal that the buffer is not full.
		full_cv.wait(lock);
	}

	if (!is_empty) {
		is_full = true;
		std::swap(active_buffer, inactive_buffer);
	}

	// Signal that the buffer is full.
	is_empty = false;
	empty_cv.notify_one();
	return active_buffer;
}

void* ThreadedExecutor::DoubleBuffer::Consume(void) {
	std::unique_lock lock(mutex);
	while (is_empty) {
		if (!running) return nullptr;
		// Wait for the producer to produce some data
		// and signal that the buffer is full.
		empty_cv.wait(lock);
	}

	if (!is_full) {
		is_empty = true;
		std::swap(active_buffer, inactive_buffer);
	}

	// Signal that the buffer is not full.
	is_full = false;
	full_cv.notify_one();
	return active_buffer;
}

void ThreadedExecutor::DoubleBuffer::Stop(void) {
	std::unique_lock lock(mutex);
	running = false;
	is_full = true;
	empty_cv.notify_all();
	full_cv.notify_all();
}
