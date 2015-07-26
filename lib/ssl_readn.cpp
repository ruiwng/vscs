/*************************************************************************
	> File Name: ssl_readn.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 20 Jul 2015 09:06:33 AM CST
 ************************************************************************/

#include  "vscs.h"

// read n characters from ssl*.

int ssl_readn(SSL *ssl, void *vptr, int n)
{
	int nleft;
	int nread;
	char *ptr = (char *)vptr;
	nleft = n;

	while(nleft > 0)
	{
		if((nread = SSL_read(ssl, ptr, nleft)) < 0)
		{
			if(errno == EINTR)
				nread = 0; /* call SSL_read() again */
			else
				return ERR;
		}else if(nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);
}
