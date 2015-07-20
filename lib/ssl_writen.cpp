/*************************************************************************
	> File Name: ssl_writen.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 20 Jul 2015 09:25:52 AM CST
 ************************************************************************/
#include  "vscs.h"

/* write n characters to SSL* */
int ssl_writen(SSL *ssl, const void *vptr, int n)
{
	int nleft;
	int nwritten;
	char *ptr = (char *)vptr;
	nleft = n;
	while(nleft > 0)
	{
		if((nwritten = SSL_write(ssl, ptr, nleft)) <= 0)
		{
			if(nwritten < 0 && errno == EINTR)
				nwritten = 0; /* and call SLL_write again */
			else
				return ERR; /* error */
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}
