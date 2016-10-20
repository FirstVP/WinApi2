// Lab2.cpp: определяет точку входа для консольного приложения.
//


#ifdef __linux__ 
#include "unistd.h"
#else
#include "process.h"
#endif

#include <functional>
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

		Worker(int number) : isEnabled(true)
		{
			threadNumber = number;
			sprintf(eventId, "%d", number);
			currentFunction = NULL;
#ifdef __linux__
			pthread_mutex_init(&activationEventMutex, NULL);
			pthread_cond_init(&activationEvent, NULL);
			condition = false;
			pthread_create(&thread, NULL, runWrapper, static_cast<void*>(this));
#else
			activationEvent = CreateEventA(NULL, TRUE, FALSE, eventId);
			thread = (HANDLE)_beginthread(&Worker::runWrapper, 0, static_cast<void*>(this));
#endif
		}

		~Worker()
		{
			isEnabled = false;
			wake();
#ifdef __linux__                        
			pthread_join(thread, NULL);
#else		
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
			CloseHandle(activationEvent);
#endif
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
#ifdef __linux__ 
			pthread_mutex_lock(&activationEventMutex);
			condition = true;
			pthread_cond_signal(&activationEvent);
			pthread_mutex_unlock(&activationEventMutex);
#else
			SetEvent(activationEvent);
#endif    
		}


	private:

#ifdef __linux__ 
		pthread_t thread;
		pthread_mutex_t activationEventMutex;
		pthread_cond_t activationEvent;
		bool condition;
#else
		HANDLE thread;
		HANDLE activationEvent;
#endif

		bool isEnabled;
		unsigned threadId;
		char eventId[256];
		tFunction currentFunction;
		int threadNumber;

		void run()
		{
			while (isEnabled)
			{
#ifdef __linux__ 
				pthread_mutex_lock(&activationEventMutex);
				while (condition == false)
				{
					pthread_cond_wait(&activationEvent, &activationEventMutex);
				}
				condition = false;
				pthread_mutex_unlock(&activationEventMutex);
#else
				WaitForSingleObject(activationEvent, INFINITE);
				ResetEvent(activationEvent);
#endif
				if (currentFunction != NULL)
					currentFunction();
				currentFunction = NULL;
			}
		}


#ifdef __linux__ 
		static void* runWrapper(void* pArguments)
		{
			static_cast<Worker*>(pArguments)->run();
		}
#else
		static void runWrapper(void* pArguments)
		{
			static_cast<Worker*>(pArguments)->run();
		}
#endif              				
	};

	typedef Worker* pWorker;

	ThreadPool(size_t threads = 1)
	{
		Logger::initializeLogger();
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
		for (int i = 0; i < workers.size(); i++)
		{
			delete workers[i];
		}
		Logger::deinitializeLogger();
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
			}
			catch (std::exception* e)
			{
				Logger::writeError("Task has been aborted");
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
			Logger::writeWarning("New worker has been added");
		}

		char message[] = "New task has been added";
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
		Sleep(1);
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
		throw new std::exception();
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
	ThreadPool* t = new ThreadPool(1);
	auto r1 = t->setTask<int>(&sum, 0, 1);
	auto r2 = t->setTask<int>(&exceptionMulti, 1);
	auto r3 = t->setTask<int>(&sum, 2, 1);
	auto r4 = t->setTask<int>(&sum, 5, 5);
	auto r5 = t->setTask<int>(&sum, 7, 7);
	auto r6 = t->setTask<int>(&sum, 10, 10);

	while ((r1->isDone != STATUS_DONE) && (r1->isDone != STATUS_ERROR))
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

	printf("R1: %d\n", r1->data);
	printf("R2: %d\n", r2->data);
	printf("R3: %d\n", r3->data);
	printf("R4: %d\n", r4->data);
	printf("R5: %d\n", r5->data);
	printf("R6: %d\n", r6->data);
	delete t;
	printf("END");
	return 0;
}