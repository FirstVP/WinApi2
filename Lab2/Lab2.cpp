// Lab2.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "process.h"
#include "easylogging++.h"
#include <queue>
#include <exception>

typedef std::function<void()> tFunction;
#define STATUS_NODATA 0
#define STATUS_DONE 1
#define STATUS_ERROR 2

template<class T>
class TaskFuture
{
public:
	int isDone;
	T data;
	TaskFuture()
	{
		isDone = STATUS_NODATA;
	}
};


class ThreadPool
{
public:

	class Worker
	{
	public:

		Worker(int number): isEnabled(true)
		{ 	
			threadNumber = number;
			sprintf(eventId, "%d", number);
			printf("%s worker\n", eventId);
			printf("%d before create\n", threadNumber);
			activationEvent = CreateEventA(NULL, TRUE, FALSE, eventId);
			printf("%d runned\n", threadNumber);
			InitializeCriticalSection(&currentFunctionSection);
			currentFunction = NULL;
			thread = (HANDLE)_beginthread (&Worker::runWrapper, 0, static_cast<void*>(this));
			threadId = GetThreadId(thread);
		}

		~Worker()
		{
			isEnabled = false;
			SetEvent(activationEvent);		
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
			CloseHandle(activationEvent);
		}

		void getRandomString(char *s, const int len) {
			static const char alphabet[] =
				"0123456789"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz";
			srand(time(0));
			for (int i = 0; i < len; ++i) {
				s[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
			}

			s[len] = 0;
		}

		bool isFree()
		{
			bool result = false;
			if (currentFunction == NULL)
				result = true;
			return result;
		}

		void setTask(tFunction function)
		{
			currentFunction = function;
		}

		void wake()
		{
			printf("%d prepare set\n", threadId);
			SetEvent(activationEvent);
			printf("%d is set\n", threadId);
		}

		bool isEnabled;
	private:
	
		HANDLE thread;
		unsigned threadId;	
		HANDLE activationEvent;
		char eventId[256];
		tFunction currentFunction;
		CRITICAL_SECTION currentFunctionSection;
		int threadNumber;

		void run()
		{		
			while (isEnabled)
			{
				printf("%d wait\n", threadId);
				WaitForSingleObject(activationEvent, INFINITE);
				printf("%d go\n", threadId);
				ResetEvent(activationEvent);
				if (currentFunction != NULL)
					currentFunction();
				currentFunction = NULL;
			}
			printf("%d finish\n", threadId);		
		}

		static void runWrapper(void* pArguments)
		{
			static_cast<Worker*>(pArguments)->run();
		}		
	};

	typedef Worker* pWorker;

	ThreadPool(size_t threads = 1)
	{
		isActive = true;
		InitializeCriticalSection(&queueSection);		
		if (threads <= 0)
			threads = 1;
		for (size_t i = 0; i<threads; i++)
		{
			pWorker pWorker(new Worker(i));
			workers.push_back(pWorker);
		}
	}

	~ThreadPool()
	{
		for (int i = 0; i < workers.size(); i++)
		{
			delete workers[i];
		}
		DeleteCriticalSection(&queueSection);
	}

	template<class R, class FN, class... ARGS>
	std::shared_ptr<TaskFuture<R>> runAsync(FN _fn, ARGS... _args)
	{
		std::function<R()> dataFunction = std::bind(_fn, _args...);
		std::shared_ptr<TaskFuture<R>> futureData(new TaskFuture<R>());
		tFunction function = [=]()
		{
			try
			{
				futureData->data = dataFunction();
				futureData->isDone = STATUS_DONE;
				printf("DONE thr %d\n", GetCurrentThreadId());
			}
			catch (std::exception* e)
			{
				futureData->isDone = STATUS_ERROR;		
			}							
		};
		appendTask(function);
		return futureData;
	}

	pWorker getFreeWorker()
	{
		for (int i = 0; i < workers.size(); i++)
		{
			if (workers[i]->isFree())
				return workers[i];
		}
		return NULL;
	}

private:
	void appendTask(tFunction function)
	{
		//EnterCriticalSection(&queueSection);
		pWorker worker = getFreeWorker();


		if (worker != NULL)
		{
			worker->setTask(function);
			worker->wake();
		}
		
		else
		{
			printf("ERROR: NO AVAIABLE THREADS");
		}
		//LeaveCriticalSection(&queueSection);
	}

	std::vector<pWorker> workers;
	std::queue<tFunction> tasks;
	CRITICAL_SECTION queueSection;
	bool isActive;
};

int sum(int a, int b)
{
	int result = 0;
	try
	{
		result = a + b;
	}
	catch (std::exception* e)
	{
		throw;
	}
	return result;
}

int exceptionMulti(int b)
{	
	try
	{
		throw new std::exception("multi");
		b = 1;
	}
	catch (std::exception* e)
	{
		throw;
	}
	return b;
}

int main(int argc, char* argv[])
{
	ThreadPool t(2);
	auto r1 = t.runAsync<int>(&sum, 0, 1);
	auto r2 = t.runAsync<int>(&sum, 0, 2);
	auto r3 = t.runAsync<int>(&exceptionMulti, 5);
	printf("PREPAREWAIT");


	while ( (r1->isDone != STATUS_DONE) && (r1->isDone != STATUS_ERROR) )
	{
		Sleep(1);
	}

	while ((r2->isDone != STATUS_DONE) && (r2->isDone != STATUS_ERROR))
	{
		Sleep(1);
	}

	while ((r3->isDone != STATUS_DONE) && (r3->isDone != STATUS_ERROR))
	{
		Sleep(1);
	}

	printf("R1: %d\n", r1->data);
	printf("R2: %d\n", r2->data);
	printf("R3: %d\n", r3->data);
	printf("Status R3: %d\n", r3->isDone);
	printf("END");


	return 0;
}