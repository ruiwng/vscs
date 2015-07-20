/*************************************************************************
	> File Name: error_log.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 11 Jul 2015 02:56:45 PM CST
 ************************************************************************/

#include  "vscs.h"
#include  <syslog.h>
#include  <stdarg.h>


static void log_doit(int errnoflag, int error, int priority, const char *fmt,
		va_list ap);

void log_open(const char *name, int option, int facility)
{
	openlog(name, option, facility);
}

/* print a message with the system's errno value and return */
void log_ret(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
}

/* print a message and terminate */
void log_sys(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}

/* Print a message and reuturn */
void log_msg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
}

/* Print a message and terminate */
void log_quit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}

/* Print a message and terminate */
void log_exit(int error, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(1, error, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}

/* Caller specifies "errnoflag" and "priority" */
static void log_doit(int errnoflag, int error, int priority, const char *fmt,
		va_list ap)
{
	char buf[MAXLINE];

	vsnprintf(buf, MAXLINE-1, fmt, ap);
	if(errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf)-1,": %s",
				strerror(error));
	strcat(buf, "\n");
	syslog(priority, "%s", buf);	
}
