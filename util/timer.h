#pragma once

#include <chrono>
#include <cstdio>

class Timer {
public:
	Timer(const char *name) : m_name(name)
	{
		m_start = std::chrono::steady_clock::now();
		m_last = m_start;
	}

	~Timer()
	{
		auto now = std::chrono::steady_clock::now();
		printf("Timer [%s]: Elapsed=%5li ms\n",
			m_name,
			std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start).count()
		);
	}

	void tick()
	{
		auto now = std::chrono::steady_clock::now();
		printf("Timer [%s]: Elapsed=%5li ms, Delta=%4li ms\n",
			m_name,
			std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start).count(),
			std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last).count()
		);
		m_last = now;
	}

private:
	const char *m_name;
	std::chrono::time_point<std::chrono::_V2::steady_clock,
		std::chrono::nanoseconds> m_start, m_last;
};
