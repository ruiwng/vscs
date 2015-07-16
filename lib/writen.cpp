/*************************************************************************
	> File Name: writen.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 12 Jul 2015 10:41:54 AM CST
 ************************************************************************/
#include  "csd.h"

/* writen n characters to fd descriptor */
int writen(int fd, const void *vptr, int n)
{
	int nleft;
	int nwritten;
	char *ptr = (char *)vptr;
	nleft = n;
	while(nleft > 0)
	{
		if((nwritten=write(fd, ptr, nleft)) <= 0)
		{
			if(nwritten < 0 && errno == EINTR)
				nwritten = 0; /* and call write() again */
			else
				return ERR; /* error */
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}
