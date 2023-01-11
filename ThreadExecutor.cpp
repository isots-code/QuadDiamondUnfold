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
	inputBufferPool(inputSize), outputBufferPool(outputSize) {
	start();
}

ThreadedExecutor::~ThreadedExecutor() {}

void* ThreadedExecutor::getInputBuffer(void) { return inputBufferPool.GetBuffer(); }

void ThreadedExecutor::queueInputBuffer(void* buffer) { inputQueue.enqueue(buffer); }

void ThreadedExecutor::returnOutputBuffer(void* buffer) { outputBufferPool.ReturnBuffer(buffer); }

void* ThreadedExecutor::dequeueOutputBuffer(void) { return outputQueue.dequeue(); }

void ThreadedExecutor::stop(void) noexcept {
	{
		std::unique_lock lock(mMutex);
		mFinished = true;
	}
	inputQueue.Stop();
	outputQueue.Stop();
	for (auto& thread : mThreads)
		thread.join();
	mCv.notify_all();
}

void ThreadedExecutor::start(void) { std::thread(&ThreadedExecutor::loop, this).detach(); }

void ThreadedExecutor::loop(void) {

	std::barrier barrier(numThreads, [&]() noexcept {
		//using namespace std::chrono;
		//auto execTime = high_resolution_clock::now(); // we check time b4 we wait for buffers as we just finished the kernel
		////doNotOptimizeAway(execTime);
		if (!mFinished) [[likely]] inputBufferPool.ReturnBuffer(input);
		if (!mFinished) [[likely]] input = inputQueue.dequeue();
		if (!mFinished) [[likely]] outputQueue.enqueue(output);
		if (!mFinished) [[likely]] output = outputBufferPool.GetBuffer();
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
	});

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

//ThreadedExecutor::DoubleBuffer::DoubleBuffer(void) : running(true), is_full(false), is_empty(true) {}
//
//void ThreadedExecutor::DoubleBuffer::setBuffers(void* curr, void* next) {
//	active_buffer = curr;
//	inactive_buffer = next;
//}
//
//void* ThreadedExecutor::DoubleBuffer::Produce(void) {
//	std::unique_lock lock(mutex);
//	while (is_full) {
//		if (!running) return nullptr;
//		// Wait for the consumer to consume some data
//		// and signal that the buffer is not full.
//		full_cv.wait(lock);
//	//// Swap the active and inactive buffers.
//		//std::swap(active_buffer, inactive_buffer);
//	}
//
//	//// Swap the active and inactive buffers.
//		//std::swap(active_buffer, inactive_buffer);
//
//	if (!is_empty) {
//		//std::swap(active_buffer, inactive_buffer);
//		is_full = true;
//	}
//
//	// Signal that the buffer is full.
//	is_empty = false;
//	empty_cv.notify_one();
//	return active_buffer;
//}
//
//void* ThreadedExecutor::DoubleBuffer::Consume(void) {
//	std::unique_lock lock(mutex);
//	while (is_empty) {
//		if (!running) return nullptr;
//		// Wait for the producer to produce some data
//		// and signal that the buffer is full.
//		empty_cv.wait(lock);
//	//// Swap the active and inactive buffers.
//		//std::swap(active_buffer, inactive_buffer);
//	}
//
//	// Swap the active and inactive buffers.
//	//std::swap(active_buffer, inactive_buffer);
//
//	if (!is_full) {
//		std::swap(active_buffer, inactive_buffer);
//		is_empty = true;
//	}
//
//	// Signal that the buffer is not full.
//	is_full = false;
//	full_cv.notify_one();
//	return active_buffer;
//}
//
//void ThreadedExecutor::DoubleBuffer::Stop(void) {
//	std::unique_lock lock(mutex);
//	running = false;
//	is_full = true;
//	empty_cv.notify_all();
//	full_cv.notify_all();
//}

ThreadedExecutor::BufferPool::BufferPool(int bufferSize) : mBufferSize(bufferSize) {}

ThreadedExecutor::BufferPool::~BufferPool() {
	// Deallocate the buffers when the pool is destroyed
	for (void* buffer : mBuffers)
		std::free(buffer);
}

void* ThreadedExecutor::BufferPool::GetBuffer() {
	std::lock_guard<std::mutex> lock(mMutex);
	// Check if there are any available buffers
	if (mAvailableBuffers.empty()) {
		// If not, allocate a new buffer and add it to the pool
		void* buffer = std::malloc(mBufferSize);
		mBuffers.push_back(buffer);
		return buffer;
	}
	// Otherwise, take a buffer from the front of the queue and return it
	void* buffer = mAvailableBuffers.front();
	mAvailableBuffers.pop_front();
	return buffer;
}

void ThreadedExecutor::BufferPool::ReturnBuffer(void* buffer) {
	std::lock_guard<std::mutex> lock(mMutex);
	if(buffer) [[likely]]
		// Add the buffer back to the pool
		mAvailableBuffers.push_back(buffer);
}

ThreadedExecutor::Queue::Queue(void) : mStopping(false) {}

void ThreadedExecutor::Queue::Stop(void) {
	{
		std::unique_lock<std::mutex> lock(mMutex);
		mStopping = true;
	}
	mCvDeq.notify_all();
	mCvEnq.notify_all();
}

void ThreadedExecutor::Queue::enqueue(void* buffer) {
	std::unique_lock<std::mutex> lock(mMutex);
	mCvEnq.wait(lock, [this] { return mQueue.size() < 10 || mStopping; });
	if (mStopping) [[unlikely]] return;
	if (buffer) [[likely]] {
		mQueue.push(buffer);
		mCvDeq.notify_one();
	}
}

void* ThreadedExecutor::Queue::dequeue() {
	std::unique_lock<std::mutex> lock(mMutex);
	mCvDeq.wait(lock, [this] { return !mQueue.empty() || mStopping; });
	if (mStopping) [[unlikely]] return nullptr;
	void* buffer = mQueue.front();
	mQueue.pop();
	mCvEnq.notify_one();
	return buffer;
}
