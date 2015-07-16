/*************************************************************************
	> File Name: master_connect.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 15 Jul 2015 01:27:50 PM CST
 ************************************************************************/
#include  "csd.h"
#include  "command_parse.h"

extern bool  stop;

// the connection to the master server.
// it was used to receive request from the master server, include remove a file, upload a file to the current 
// slave server and download a file from the slave server.
void *master_connect_thread(void *arg)
{
	int sock_fd = (int)arg;
	char recvline[MAXBUF];
	int nread;
	while(!stop)
	{		
		if((nread = read(sock_fd, recvline, MAXBUF)) < 0)// receive command from the master server
		{
			if(errno == EINTR)
			{
				if(!stop)// read function was interrupted by signals  
					continue;
				else// the slave exist
					break;
			}
			else
				log_ret("master connect_thread: read error");
		}
		else if(nread == 0) // the connection to the current master was broken
			break;
		else// parse the command
			command_parse(recvline);
	}
	struct sockaddr_in master_addr;
	socklen_t len = sizeof(master_addr);
	if(getpeername(sock_fd, (sockaddr*)&master_addr, &len) < 0)
		log_msg("master_connect_thread: getpeername error");
	else
	{
		char addr[MAXLINE];
		inet_ntop(AF_INET, (void *)&master_addr.sin_addr, addr, MAXLINE);
		log_msg("master_connect_thread: %s was connected", addr);
	}
	close(sock_fd);
	return NULL;
}


