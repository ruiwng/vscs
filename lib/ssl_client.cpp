/*************************************************************************
	> File Name: ssl_client.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 09:32:42 AM CST
 ************************************************************************/
#include "vscs.h"

// if success return SSL*, otherwise return NULL
SSL* ssl_client(SSL_CTX *ctx, int sockfd)
{
	SSL *ssl = SSL_new(ctx);

	if(ssl == NULL)
	{
		log_msg("ssl_client: SSL_new error");
		return NULL;
	}

	SSL_set_fd(ssl, sockfd);
	int ret = SSL_connect(ssl);

	if(ret == -1)
	{
		log_msg("ssl_client: SSL_connect error");
		return NULL;
	}

	return ssl;
}
