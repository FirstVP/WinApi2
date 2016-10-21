#include "Logger.h"
#include "easylogging++.h"

CRITICAL_SECTION Logger::outputSection;

Logger::Logger()
{
}


Logger::~Logger()
{
}

void Logger::initializeLogger()
{
	el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Filename, "error.log");
	Logger::turnOffFileOutput();
#ifdef __linux__ 
	pthread_mutex_init(&outputSection, NULL);
#else
	InitializeCriticalSection(&outputSection);
#endif
}
void Logger::deinitializeLogger()
{
	DeleteCriticalSection(&outputSection);
}
void Logger::writeMessage(char* message)
{
	EnterCriticalSection(&outputSection);
	LOG(INFO) << message;
	LeaveCriticalSection(&outputSection);
}
void Logger::writeWarning(char* message)
{
	EnterCriticalSection(&outputSection);
	LOG(WARNING) << message;
	LeaveCriticalSection(&outputSection);
}
void Logger::writeError(char* message)
{
	EnterCriticalSection(&outputSection);
	LOG(ERROR) << message;
	LeaveCriticalSection(&outputSection);
}

void Logger::turnOffFileOutput()
{
	//el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "false");
}

void Logger::turnOnFileOutput()
{
	//el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "true");
}
