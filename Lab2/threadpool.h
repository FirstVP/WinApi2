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
#define MAX_THREADS 2

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

	ThreadPool(size_t threads)
	{
		Logger::initializeLogger();
		if (threads <= 0)
			threads = 1;
		else if (threads > MAX_THREADS)
			threads = MAX_THREADS;
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

	private:

	pWorker getFreeWorker()
	{
		for (int i = 0; i < workers.size(); i++)
		{
			if (workers[i]->isFree())
				return workers[i];
		}
		return NULL;
	}

	void appendTask(tFunction function)
	{
		pWorker worker = getFreeWorker();
		if (worker == NULL)
		{
			worker = createNewWorker();
			if (workers.size() <= MAX_THREADS)
				Logger::writeMessage("New worker has been added");		
			else			
				Logger::writeWarning("Pool size limit reached, but new worker has been added");
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
	//std::queue<tFunction> tasks;
};