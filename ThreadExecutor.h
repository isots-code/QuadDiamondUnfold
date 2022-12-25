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

class ThreadedExecutor {
public:

	explicit ThreadedExecutor(const std::size_t dim, const std::size_t numThreads = 1)
		: dim(dim), numThreads(numThreads), mStopped(false), mRunning(true) {
		start();
	}

	~ThreadedExecutor() { stop(); }

	auto readOutput(void) { return outputBuf.read(); }

	auto writeInput(void) { return inputBuf.write(); }

	void setIOBuffers(void* inCurr, void* inNext, void* outCurr, void* outNext) {
		setIBuffers(inCurr, inNext);
		setOBuffers(outCurr, outNext);
	}

	void setIBuffers(void* inCurr, void* inNext) {
		inputBuf.setBuffers(inCurr, inNext);
	}

	void setOBuffers(void* outCurr, void* outNext) {
		outputBuf.setBuffers(outCurr, outNext);
	}

	void stop(void) noexcept {
		mRunning = false;
		mRunning.notify_all();
		mStopped.wait(false); //blocks while threads are closing
	}

	virtual void kernel(const int id) = 0;

protected:

	void* output;
	const void* input;
	const std::size_t dim;
	const std::size_t numThreads;
	std::atomic_bool mStopped;
	std::atomic_bool mRunning;
	std::vector<std::thread> mThreads;

	void start() { std::thread(&ThreadedExecutor::loop, this).detach(); }

private:
	void loop(void) {

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

		for (auto i = 0u; i < numThreads; i++) {
			mThreads.emplace_back([&](int j) {
				while (true) {
					mainBarrier.arrive_and_wait();
					if (!mRunning) [[unlikely]] break; //exit early if we where told to stop
					kernel(j);
				}
				mainBarrier.arrive_and_drop(); //we have to resync
			}, i);
		}

		//stopping code moved here
		mRunning.wait(true); //blocks if we're still running
		inputBuf.signalStop();
		outputBuf.signalStop();

		for (auto& thread : mThreads)
			thread.join();
		mStopped = true;
		mStopped.notify_all();

	}

protected:
	class DoubleBuffer {
	public:

		DoubleBuffer(void) : readValid(false), writeValid(true) {}

		void* write(void) {
			if (stopExit) return nullptr; //early exit, just in case
			void* retPtr;
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

		const void* read(void) {
			if (stopExit) return nullptr; //early exit, just in case
			void* retPtr;
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

		void setBuffers(void* curr, void* nxt) {
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