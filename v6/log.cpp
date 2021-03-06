#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include "log.hpp"

/* Allocating and initializing Logger's static data member
 * The pointer is being allocated, not the object itself
 */
Logger *Logger::s_instance = 0;

void Logger::debug_log(const char *ident, const char *func, const char *fmt, ...)
{
	std::time_t t = std::time(NULL);
	char time_buf[100];
	std::strftime(time_buf, sizeof time_buf, "%b %d %H:%M:%S", std::gmtime(&t));
	va_list args1;
	va_start(args1, fmt);
	va_list args2;
	va_copy(args2, args1);
	std::vector<char> buf(1+std::vsnprintf(NULL, 0, fmt, args1));
	va_end(args1);
	std::vsnprintf(buf.data(), buf.size(), fmt, args2);
	va_end(args2);
	std::printf("%s %s: %s: %s", ident, time_buf, func, buf.data());
}
