/*************************************************************************
	> File Name: daemonize.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 12 Jul 2015 09:10:52 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  <syslog.h>
#include  <fcntl.h>
#include  <sys/resource.h>

void daemonize(const char *cmd)
{
	/*
	 * Clear file creation mask.
	 */
	umask(0);

	// Get maximum number of file descriptors.
	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
		log_quit("%s: can't get file limit", cmd);

	/* Become a session leader to lose controlling TTY.*/
	int pid;
	if((pid = fork()) < 0)
		log_quit("%s: can't fork", cmd);
	else if(pid != 0) /* parent */
		exit(0);
	setsid();

	/* 
	 * Ensure future opens won't allocate controlling TTYs
	 */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP, &sa, NULL) < 0)
		log_quit("%s: can't ignore SIGHUP", cmd);
	if((pid = fork()) < 0)
		log_quit("%s: can't fork", cmd);
	else if(pid != 0) /* parent */
		exit(0);

	/*
	 * Change the current working directory to the root so
	 * we won't prevent file system from being unmounted.
	 */
	if(chdir("/") < 0)
		log_quit("%s: can't change directory to /", cmd);

	/*
	 * Close all open file descriptors.
	 */
	if(rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for(unsigned int i = 0; i< 3;++i)
		close(i);

	/*
	 * Attach file descriptors 0, 1, and 2 to /dev/null.
	 */
	
	int fd0 = open("/dev/null", O_RDWR);
	int fd1 = dup(0);
	int fd2 = dup(0);

	/*
	 * Initialize the log file.
	 */
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if(fd0 != 0 || fd1 != 1 || fd2 != 2)
	{
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d",
				fd0, fd1, fd2);
		exit(1);
	}
}
