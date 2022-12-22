#pragma once
#include <iostream>
#include <cstddef>
#include <barrier>
#include <atomic>
#include <chrono>
#include <memory>
#include <vector>
#include <thread>
#include <latch>
#include <mutex>

template <class T>
inline auto doNotOptimizeAway(T const& datum) {
	return reinterpret_cast<char const volatile&>(datum);
}

template <class T>
class ThreadedExecutor {
public:

	explicit ThreadedExecutor(const std::size_t dim, const std::size_t numThreads = 1)
		: dim(dim), numThreads(numThreads), mStopped(false), mRunning(true) {
		start();
	}

	~ThreadedExecutor() { stop(); }

	auto fakeReadOutput(void) { return outputBuf.read(); }

	auto fakeWriteInput(void) { return inputBuf.write(); }

	void setIOBuffers(T* inCurr, T* inNext, T* outCurr, T* outNext) {
		inputBuf.setBuffers(inCurr, inNext);
		outputBuf.setBuffers(outCurr, outNext);
	}

	void stop(void) noexcept {
		mRunning = false;
		mRunning.notify_all();
		mStopped.wait(false); //blocks while threads are closing
	}

	virtual void kernel(const int id) = 0;

protected:

	T* output;
	const T* input;
	const std::size_t dim;
	const std::size_t numThreads;
	std::atomic_bool mStopped;
	std::atomic_bool mRunning;
	std::vector<std::thread> mThreads;
	class DoubleBuffer;
	DoubleBuffer inputBuf;
	DoubleBuffer outputBuf;

	void start() {
		std::thread parent([this] {

			std::barrier mainBarrier(numThreads, [&]() noexcept {
				using namespace std::chrono;
				auto execTime = high_resolution_clock::now(); // we check time b4 we wait for buffers as we just finished the kernel
				//doNotOptimizeAway(execTime);
				if (mRunning) [[likely]] inputBuf.swap();
				if (mRunning) [[likely]] input = inputBuf.read();
				if (mRunning) [[likely]] output = outputBuf.write();
				if (mRunning) [[likely]] outputBuf.swap();
				if (!mRunning) [[unlikely]] return;
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
			});

			for (auto i = 0u; i < numThreads; ++i) {
				mThreads.emplace_back([&]() {
					while (true) {
						mainBarrier.arrive_and_wait();
						if (!mRunning) [[unlikely]] break; //exit early if we where told to stop
						kernel(i);
					}
					mainBarrier.arrive_and_drop(); //we have to resync
				});
			}

			//stopping code moved here
			mRunning.wait(true); //blocks if we're still running
			inputBuf.signalStop();
			outputBuf.signalStop();

			for (auto& thread : mThreads)
				thread.join();
			mStopped = true;
			mStopped.notify_all();

		});
		parent.detach();
	}

protected:
	class DoubleBuffer {
	public:

		DoubleBuffer(void) : readValid(false), writeValid(true) {}

		T* write(void) {
			if (stopExit) return nullptr; //early exit, just in case
			T* retPtr;
			writeValid.wait(false); //blocks if not valid (false)
			//writeValid.wait(false, std::memory_order_acquire); //blocks if not valid (false)
			if (stopExit) return nullptr; //post signal exit
			{
				std::lock_guard<std::mutex> lk(stateMtx); //lock state
				writeValid = false; //next op is invalid
				//writeValid.store(false, std::memory_order_release); //next op is invalid
				writeValid.notify_all();
				retPtr = next;
			}
			return retPtr;
		}

		const T* read(void) {
			if (stopExit) return nullptr; //early exit, just in case
			T* retPtr;
			readValid.wait(false); //blocks if not valid (false)
			//readValid.wait(false, std::memory_order_acquire); //blocks if not valid (false)
			if (stopExit) return nullptr; //post signal exit
			{
				std::lock_guard<std::mutex> lk(stateMtx); //lock state
				readValid = false; //next op is invalid
				//readValid.store(false, std::memory_order_release); //next op is invalid
				readValid.notify_all();
				retPtr = current;
			}
			return retPtr;
		}

		void swap(void) {
			if (stopExit) return; //early exit, just in case
			writeValid.wait(true); //blocks if theres a valid op to do
			readValid.wait(true); //blocks if theres a valid op to do
			//writeValid.wait(true, std::memory_order_acquire); //blocks if theres a valid op to do
			//readValid.wait(true, std::memory_order_acquire); //blocks if theres a valid op to do
			if (stopExit) return; //post signal exit
			{
				std::lock_guard<std::mutex> lk(stateMtx); //lock state
				std::swap(current, next);
				readValid = true; //next op is valid
				writeValid = true; //next op is valid
				//readValid.store(true, std::memory_order_release); //next op is valid
				//writeValid.store(true, std::memory_order_release); //next op is valid
				readValid.notify_all();
				writeValid.notify_all();
			}
		}

		void setBuffers(T* curr, T* nxt) {
			current = curr;
			next = nxt;
		}

		void signalStop(void) {
			stopExit = true;
			//1st we say we have valid ops, to leave the read/write functions
			readValid = true;
			writeValid = true;
			//readValid.store(true, std::memory_order_release);
			//writeValid.store(true, std::memory_order_release);
			readValid.notify_all();
			writeValid.notify_all();
			std::this_thread::sleep_for(std::chrono::milliseconds{ 100 }); //give time for threads to react (shrug)
			//2nd we say we have done the ops, to leave the swap function
			readValid = false;
			writeValid = false;
			//readValid.store(false, std::memory_order_release);
			//writeValid.store(false, std::memory_order_release);
			readValid.notify_all();
			writeValid.notify_all();
		}

	private:
		T* next = nullptr;
		T* current = nullptr;
		std::mutex stateMtx;
		bool stopExit = false;
		std::atomic_bool readValid;
		std::atomic_bool writeValid;
	};

};