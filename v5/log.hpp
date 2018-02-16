#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <iostream>

enum {
	ERROR,  /* error conditions */
	INFO,   /* informational message */
	DEBUG   /* debug-level message */
};

#define call_log(_id,_str, arg...)\
	do {\
		Logger::instance()->debug_log(#_id, "%s : "_str, __FUNCTION__, ##arg);\
	} while(0)

#define log_info(_str, arg...)           call_log(INFO, _str, ##arg)
#define log_err(_str, arg...)            call_log(ERROR, _str, ##arg)
#define log_debug(_str, arg...)          call_log(DEBUG, _str, ##arg)

class Logger {
private:
	int log_level;
	static Logger *s_instance;
	Logger() {
		log_level = DEBUG;
	}
public:
	void set_log_level(int level);
	void debug_log(const char *ident, const char *fmt, ...);
	static Logger *instance()
	{
		if(!s_instance)
			s_instance = new Logger;
		return s_instance;
	}
};

#endif

