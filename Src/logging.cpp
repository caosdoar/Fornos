/*
Copyright 2018 Oscar Sebio Cajaraville

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "logging.h"
#include <iostream>

static std::string logBuffer;
static bool logBufferEnabled = true;

#define DEBUG 0

#if _WIN32 && DEBUG
#include <Windows.h>
#endif

namespace
{
	std::string makeString(const std::string &lvl, const std::string mod, const std::string msg)
	{
		return lvl + " [" + mod + "] " + msg + "\n";
	}
}

void logDebug(const std::string &module, const std::string &msg)
{
	const auto str = makeString("DEBG", module, msg);
#if _WIN32 && DEBUG
	OutputDebugString(str.c_str());
#else
	std::cout << str;
#endif
	if (logBufferEnabled) logBuffer += str;
}

void logWarning(const std::string &module, const std::string &msg)
{
	const auto str = makeString("WARN", module, msg);
#if _WIN32 && DEBUG
	OutputDebugString(str.c_str());
#else
	std::cout << str;
#endif
	if (logBufferEnabled) logBuffer += str;
}

void logError(const std::string &module, const std::string &msg)
{
	const auto str = makeString("ERRO", module, msg);
#if _WIN32 && DEBUG
	OutputDebugString(str.c_str());
#else
	std::cerr << str;
#endif
	if (logBufferEnabled) logBuffer += str;
}

void enableLogBuffer()
{
	logBufferEnabled = true;
}

void disableLogBuffer()
{
	logBufferEnabled = false;
	logBuffer.clear();
}

void clearLogBuffer()
{
	logBuffer.clear();
}

const std::string& getLogBuffer()
{
	return logBuffer;
}