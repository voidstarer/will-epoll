#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <iostream>

enum {
	ERROR,  /* error conditions */
	INFO,   /* informational message */
	DEBUG   /* debug-level message */
};

#define call_log(_id, _str, arg...)\
	do {\
		if(_id <= Logger::instance()->log_level) { Logger::instance()->debug_log(#_id, __FUNCTION__, _str, ##arg);}\
	} while(0)

#define log_info(_str, arg...)           call_log(INFO, _str, ##arg)
#define log_err(_str, arg...)            call_log(ERROR, _str, ##arg)
#define log_debug(_str, arg...)          call_log(DEBUG, _str, ##arg)

#define set_log_level(_l)  Logger::instance()->log_level = (_l);

class Logger {
private:
	static Logger *s_instance;
	Logger() {
		log_level = DEBUG;
	}
public:
	int log_level;
	void debug_log(const char *ident, const char *func, const char *fmt, ...) __attribute__((format(printf, 4, 5)));
	static Logger *instance()
	{
		if(!s_instance)
			s_instance = new Logger;
		return s_instance;
	}
};

#endif

