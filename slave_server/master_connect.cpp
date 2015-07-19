/*************************************************************************
	> File Name: master_connect.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 15 Jul 2015 01:27:50 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "command_parse.h"

extern SSL_CTX *ctx; //used for SSL

// the connection to the master server.
// it was used to receive request from the master server, include remove a file, upload a file to the current 
// slave server and download a file from the slave server.

void *master_connect_thread(void *arg)
{
	struct sockaddr_in master_addr;
	socklen_t len = sizeof(master_addr);
	int sock_fd = (int)arg;

	char addr[MAXLINE];
	if(getpeername(sock_fd, (sockaddr*)&master_addr, &len) < 0)
		log_msg("mater_connect_thread: getpeername error");
	else
		log_msg("master_connect_thread: %s was connected", 
				inet_ntop(AF_UNSPEC, (void *)&master_addr.sin_addr, addr, MAXLINE));
	SSL *ssl = ssl_server(ctx, sock_fd);

	if(ssl == NULL)
	{
		log_quit("master_connect_thread: ssl_server error");
		return NULL;
	}
	ssl = SSL_new(ctx);

	char recvline[MAXBUF + 1];
	int nread;
	while(true)
	{		
		if((nread = SSL_read(ssl, recvline, MAXBUF)) < 0)// receive command from the master server
		{
			log_ret("master connect_thread: read error");
			continue;
		}
		else if(nread == 0) // the connection to the current master was broken
			break;
		else// parse the command
		{
			recvline[nread] = '\0';
			command_parse(recvline);
		}
	}
	SSL_shutdown(ssl); /* send SSL/TLC close_notify */
	close(sock_fd); //close the socket connected to the master server
	SSL_free(ssl); // free the ssl 
	log_msg("master_connect_thread: %s was unconnected", addr);
	return NULL;
}


