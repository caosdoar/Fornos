#pragma once

#include <chrono>

class Timing
{
public:
	void begin() { _begin = std::chrono::high_resolution_clock::now(); }
	void end() { _end = std::chrono::high_resolution_clock::now(); }
	double elapsedSeconds() const 
	{ 
		return std::chrono::duration_cast<std::chrono::milliseconds>(_end - _begin).count() / 1000.0; 
	}

private:
	std::chrono::high_resolution_clock::time_point _begin;
	std::chrono::high_resolution_clock::time_point _end;
};