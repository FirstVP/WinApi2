#pragma once

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

template<class T>
struct TaskFuture
{
	TaskFuture() :isDone(false)
	{
	}
	bool isDone;
	T data;
};

class ThreadPool
{
public:

	typedef std::function<void()> tFunction;

	class Worker
	{
	public:

		Worker()
			:enabled(true), fqueue()
			, thread(&Worker::run, this)
		{}

		~Worker()
		{
			enabled = false;
			cv.notify_one();
			thread.join();
		}

		void appendFn(tFunction fn)
		{
			std::unique_lock<std::mutex> locker(mutex);
			fqueue.push(fn);
			cv.notify_one();
		}

		size_t getTaskCount()
		{
			std::unique_lock<std::mutex> locker(mutex);
			return fqueue.size();
		}

		bool   isEmpty()
		{
			std::unique_lock<std::mutex> locker(mutex);
			return fqueue.empty();
		}

	private:

		bool					enabled;
		std::condition_variable cv;
		std::queue<tFunction>		fqueue;
		std::mutex				mutex;
		std::thread				thread;

		void run()
		{
			while (enabled)
			{
				std::unique_lock<std::mutex> locker(mutex);

				cv.wait(locker, [&]() { return !fqueue.empty() || !enabled; });
				while (!fqueue.empty())
				{
					tFunction fn = fqueue.front();

					locker.unlock();
					fn();

					locker.lock();
					fqueue.pop();
				}
			}
		}
	};

	typedef std::shared_ptr<Worker> pWorker;

	ThreadPool(size_t threads = 1)
	{
		if (threads == 0)
			threads = 1;
		for (size_t i = 0; i<threads; i++)
		{
			pWorker pWorker(new Worker);
			workers.push_back(pWorker);
		}
	}

	~ThreadPool() {}

	template<class _R, class _FN, class... _ARGS>
	std::shared_ptr<TaskFuture<_R>> runAsync(_FN _fn, _ARGS... _args)
	{
		std::function<_R()> rfn = std::bind(_fn, _args...);
		std::shared_ptr<TaskFuture<_R>> pData(new TaskFuture<_R>());
		tFunction fn = [=]()
		{
			pData->data = rfn();
			pData->isDone = true;
		};
		auto pWorker = getFreeWorker();
		pWorker->appendFn(fn);
		return pData;
	}

	template<class _FN, class... _ARGS>
	void runAsync(_FN _fn, _ARGS... _args)
	{
		auto pWorker = getFreeWorker();
		pWorker->appendFn(std::bind(_fn, _args...));
	}

private:

	pWorker getFreeWorker()
	{
		pWorker pWorker;
		size_t minTasks = UINT32_MAX;
		for (auto &it : workers)
		{
			if (it->isEmpty())
			{
				return it;
			}
			else if (minTasks > it->getTaskCount())
			{
				minTasks = it->getTaskCount();
				pWorker = it;
			}
		}
		return pWorker;
	}

	std::vector<pWorker> workers;


};

#endif /*_THREADPOOL_H_*/