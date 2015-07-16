/*************************************************************************
	> File Name: slave_server.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 07:52:21 PM CST
 ************************************************************************/
#include  "csd.h"
#include  "master_connect.h"

bool stop = false;

void sig_intr(int signo)// the handler of the interrupt signal.
{
	stop = true;
}


int main(int argc, char **argv)
{
	int listen_fd;
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		log_sys("main: socket error");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SLAVE_PORT);

	if(bind(listen_fd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		log_sys("main: bind error");
	if(listen(listen_fd, LISTEN_COUNT) < 0)
		log_sys("main: listen error");
	
	daemonize("slave_server");

	if(signal(SIGINT, sig_intr) == SIG_ERR) // install the handler for the interrupt signal.
		log_sys("signal error");
	
	pthread_t thread;
	while(!stop)
	{
		struct sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		int connfd;
		if((connfd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
		{
			if(errno == EINTR)
			{
				if(!stop)// accept was interrupted by other signals
				    continue; /* back to while */
				else
				{
					log_msg("main: slave exist");
					break;
				}
			}
			else
				log_sys("main: accept error");
		}
		pthread_create(&thread, NULL, master_connect_thread, (void *)connfd);			
	}
	return 0;
}
