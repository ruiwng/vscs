/*************************************************************************
	> File Name: ssl_init.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 17 Jul 2015 07:50:52 PM CST
 ************************************************************************/
#include  "vscs.h"

SSL_CTX *ssl_init(const char *certificate, const char *key)
{
	SSL_CTX *ctx;
	const SSL_METHOD *meth;

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	meth = SSLv23_server_method();

	ctx = SSL_CTX_new(meth);

	if(!ctx)
	{
		log_msg("ssl_init: SSL_CTX_new error.");
		return NULL;
	}
	//read the certificate file
	if(SSL_CTX_use_certificate_file(ctx, certificate, SSL_FILETYPE_PEM) <= 0)
	{
		log_msg("ssl_init: SSL_CTX_use_certificate_file error");
		return NULL;
	}
	//read the private key file
	if(SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0)
	{
		log_msg("ssl_init: SSL_CTX_check_private_key error.");
		return NULL;
	}

	return ctx;
}
