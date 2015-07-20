/*************************************************************************
	> File Name: ssl_server.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 09:27:11 AM CST
 ************************************************************************/
#include  "vscs.h"

// if success return SSL*, otherwise return NULL
SSL *ssl_server(SSL_CTX *ctx, int sockfd)
{
	SSL *ssl = SSL_new(ctx);

	if(ssl == NULL)
	{
		log_msg("ssl_server: SSL_new error");
		return NULL;
	}

	SSL_set_fd(ssl, sockfd);
	int ret = SSL_accept(ssl);

	if(ret == -1)
	{
		log_msg("ssl_server: SSL_accept error");
		return NULL;
	}

	return ssl;
}
