// Lab2.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "process.h"
#include "easylogging++.h"
#include <queue>
#include <exception>
#include "Logger.h"

INITIALIZE_EASYLOGGINGPP

typedef std::function<void()> tFunction;
#define STATUS_NODATA 0
#define STATUS_DONE 1
#define STATUS_ERROR 2

#define BUFFER_SIZE 256

class ThreadPool
{
public:
	
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

	class Worker
	{
	public:

		Worker(int number): isEnabled(true)
		{ 	
			threadNumber = number;
			sprintf(eventId, "%d", number);
			activationEvent = CreateEventA(NULL, TRUE, FALSE, eventId);
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

		int getThreadNumber()
		{
			return threadNumber;
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
			SetEvent(activationEvent);
			printf("%d is set\n", threadNumber);
		}

		
	private:
		bool isEnabled;
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
				//printf("%d wait\n", threadId);
				WaitForSingleObject(activationEvent, INFINITE);
				//printf("%d go\n", threadId);
				ResetEvent(activationEvent);
				if (currentFunction != NULL)
					currentFunction();
				currentFunction = NULL;
			}
			//printf("%d finish\n", threadId);		
		}

		static void runWrapper(void* pArguments)
		{
			static_cast<Worker*>(pArguments)->run();
		}		
	};

	typedef Worker* pWorker;

	ThreadPool(size_t threads = 1)
	{	
		if (threads <= 0)
			threads = 1;
		for (size_t i = 0; i<threads; i++)
		{
			pWorker pWorker(new Worker(i));
			workers.push_back(pWorker);
		}

		char message[] = "Initialization, %d threads have been created";
		char buffer[BUFFER_SIZE];
		sprintf(buffer, message, workers.size());
		Logger::writeMessage(buffer);
	}

	~ThreadPool()
	{
		printf("ALL THREADS %d\n", workers.size());
		for (int i = 0; i < workers.size(); i++)
		{
			delete workers[i];
		}
	}

	template<class R, class FN, class... ARGS>
	std::shared_ptr<TaskFuture<R>> setTask(FN _fn, ARGS... _args)
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
		pWorker worker = getFreeWorker();
		if (worker == NULL)
		{
			worker = createNewWorker();
		}

		char message[] = "New task has been added (%d thread)";
		char buffer[BUFFER_SIZE];
		sprintf(buffer, message, worker->getThreadNumber());
		Logger::writeMessage(buffer);

		worker->setTask(function);
		worker->wake();
	}

	pWorker createNewWorker()
	{
		pWorker pWorker(new Worker(workers.size()));
		workers.push_back(pWorker);
		return pWorker;
	}

	std::vector<pWorker> workers;
	std::queue<tFunction> tasks;
};

int sum(int a, int b)
{
	int result = 0;
	try
	{
		Sleep(1000);
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
	Logger::initializeLogger();
	ThreadPool t(1);
	auto r1 = t.setTask<int>(&sum, 0, 1);
	auto r2 = t.setTask<int>(&sum, 0, 2);
	auto r3 = t.setTask<int>(&exceptionMulti, 5);
	auto r4 = t.setTask<int>(&sum, 10, 90);
	auto r5 = t.setTask<int>(&sum, 11, 55);
	auto r6 = t.setTask<int>(&sum, 11, 55);
	auto r7 = t.setTask<int>(&sum, 11, 55);

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

	while ((r4->isDone != STATUS_DONE) && (r4->isDone != STATUS_ERROR))
	{
		Sleep(1);
	}

	while ((r5->isDone != STATUS_DONE) && (r5->isDone != STATUS_ERROR))
	{
		Sleep(1);
	}

	while ((r6->isDone != STATUS_DONE) && (r6->isDone != STATUS_ERROR))
	{
		Sleep(1);
	}

	while ((r7->isDone != STATUS_DONE) && (r7->isDone != STATUS_ERROR))
	{
		Sleep(1);
	}

	printf("R1: %d\n", r1->data);
	printf("R2: %d\n", r2->data);
	printf("R3: %d\n", r3->data);
	printf("R4: %d\n", r4->data);
	printf("R5: %d\n", r5->data);
	printf("R6: %d\n", r6->data);
	printf("R7: %d\n", r7->data);
	printf("Status R3: %d\n", r3->isDone);
	
	printf("END");
	Logger::deinitializeLogger();
	return 0;
}