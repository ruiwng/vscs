/*************************************************************************
	> File Name: cluster_status.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 26 Jul 2015 01:44:44 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "cluster_status.h"

extern SSL_CTX *ctx_client;
extern char master_status_port[MAXLINE];

void cluster_status()
{
	int sockfd = client_connect("127.0.0.1", master_status_port);
	if(sockfd == -1)
	{
		printf("master server not start.\n");
		return;
	}
	SSL *ssl = ssl_client(ctx_client, sockfd);
	char message[MAXLINE + 1];
	while(true)
	{
		int len = SSL_read(ssl , message, MAXLINE);
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
	SSL_shutdown(ssl);
	close(sockfd);
	SSL_free(ssl);
	return;
}
