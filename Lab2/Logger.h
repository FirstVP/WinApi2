#pragma once
#include "easylogging++.h"

#ifdef __linux__ 
#define CRITICAL_SECTION pthread_mutex_t
#define DeleteCriticalSection pthread_mutex_destroy
#define EnterCriticalSection pthread_mutex_lock
#define LeaveCriticalSection pthread_mutex_unlock
#endif

class Logger
{
public:
	Logger();
	~Logger();
	static void initializeLogger();
	static void deinitializeLogger();
	static void writeMessage(char * message);
	static void writeWarning(char * message);
	static void writeError(char * message);
	static void turnOffFileOutput();
	static void turnOnFileOutput();
private:
	static CRITICAL_SECTION outputSection;
};

