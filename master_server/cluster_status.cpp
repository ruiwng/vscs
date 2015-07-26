/*************************************************************************
	> File Name: cluster_status.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 26 Jul 2015 01:44:44 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "cluster_status.h"

void cluster_status(const char *master_status_port)
{
	int sockfd = client_connect("127.0.0.1", master_status_port);
	if(sockfd == -1)
	{
		printf("master server not start.\n");
		return;
	}
	char message[MAXLINE + 1];
	while(true)
	{
		int len = read(sockfd, message, MAXLINE);
		if(len < 0)
		{
			if(errno == EINTR)
				continue;
		}
		else if(len == 0)
			break;
		message[len] = '\0';
		printf("%s", message);
	}
	close(sockfd);
	return;
}
