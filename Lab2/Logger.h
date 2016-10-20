#pragma once
#include "easylogging++.h"

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
private:
	static CRITICAL_SECTION outputSection;
};

