#pragma once

#include <cctype>
#include <cstdint>
#include <iostream> // cout
#include "vector.h"

typedef Vector2D<int16_t> v2s16;
typedef Vector2D<uint16_t> v2u16;

// Change to 1 for verbose logging
#if 0
	#define VERBOSE(str) \
		std::cout << "\e[0;37m" << __PRETTY_FUNCTION__ << "\e[0m: " << str << std::endl
#else
	#define VERBOSE(str)
#endif

#define LOG(str) \
	std::cout << "\e[0;36m" << __PRETTY_FUNCTION__ << "\e[0m: " << str << std::endl

#define WARN(str) \
	std::cout << "\e[1;33m" << __PRETTY_FUNCTION__ << "\e[0m: " << str << std::endl

#define ERROR(str) \
	{ std::cout << "\e[0;31m" << __PRETTY_FUNCTION__ << "\e[0m: " << str << std::endl; \
	exit(EXIT_FAILURE); }

#define ASSERT(expr) \
	if (!(expr)) { \
		std::cout << "\e[0;31m" << #expr << ": Assertion failed\e[0m" << std::endl; \
		throw "Assertion failed"; \
	}

#define RANGELIM(var, min, max) \
	((var < min) ? min : ((var > max) ? max : var))

#define ABS(var) (var < 0 ? -(var) : var)

#define PP(var) "(" << (var).X << ", " << (var).Y << ")"
