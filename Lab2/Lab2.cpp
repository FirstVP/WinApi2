// Lab2.cpp: определяет точку входа для консольного приложения.
//

#include "ThreadPool.h"
#include "time.h"

#ifdef __linux__ 
#define WAIT usleep (1000)
#else
#define WAIT Sleep(1)
#endif

int multiplicate(int a, int b)
{
	int result = 0;
	srand(time(0));
	try
	{
		auto time = rand();
		LOG(INFO) << time << "\n";
		Sleep(time);
		result = a * b;
	}
	catch (std::exception* e)
	{
		throw;
	}
	return result;
}

int sum(int a, int b)
{
	int result = 0;
	srand(time(0));
	try
	{
		auto time = rand();
		LOG(INFO) << time << "\n";
		Sleep(time);
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
	auto r4 = t->setTask<int>(&multiplicate, 5, 5);
	auto r5 = t->setTask<int>(&sum, 7, 7);
	auto r6 = t->setTask<int>(&sum, 10, 10);

	while ((r1->isDone != STATUS_DONE) && (r1->isDone != STATUS_ERROR))
	{
		WAIT;
	}

	while ((r2->isDone != STATUS_DONE) && (r2->isDone != STATUS_ERROR))
	{
		WAIT;
	}

	while ((r3->isDone != STATUS_DONE) && (r3->isDone != STATUS_ERROR))
	{
		WAIT;
	}

	while ((r4->isDone != STATUS_DONE) && (r4->isDone != STATUS_ERROR))
	{
		WAIT;
	}

	while ((r5->isDone != STATUS_DONE) && (r5->isDone != STATUS_ERROR))
	{
		WAIT;
	}

	while ((r6->isDone != STATUS_DONE) && (r6->isDone != STATUS_ERROR))
	{
		WAIT;
	}

	printf("R1: %d, status: %d\n", r1->data, r1->isDone);
	printf("R2: %d, status: %d\n", r2->data, r2->isDone);
	printf("R3: %d, status: %d\n", r3->data, r3->isDone);
	printf("R4: %d, status: %d\n", r4->data, r4->isDone);
	printf("R5: %d, status: %d\n", r5->data, r5->isDone);
	printf("R6: %d, status: %d\n", r6->data, r6->isDone);
	delete t;
	printf("END");
	return 0;
}