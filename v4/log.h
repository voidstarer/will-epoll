#ifndef _LOG_H_
#define _LOG_H_

enum {
        ERROR,  /* error conditions */
        INFO,   /* informational message */
        DEBUG   /* debug-level message */
};

extern int log_level;

#define call_log(_id,_fd,_str, arg...)\
	do {\
		if(_id <= log_level) log_me(_fd, #_id, "%s : "_str, __FUNCTION__, ##arg);\
	} while(0)

#define log_info(_str, arg...)           call_log(INFO, stdout, _str, ##arg)
#define log_err(_str, arg...)            call_log(ERROR, stderr, _str, ##arg)
#define log_debug(_str, arg...)          call_log(DEBUG, stdout, _str, ##arg)

void set_log_level(int level);

extern void log_me(FILE *fd, const char *ident, const char *fmt, ...);

#endif

