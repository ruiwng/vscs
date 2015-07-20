/*************************************************************************
	> File Name: readn.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 12 Jul 2015 10:35:46 AM CST
 ************************************************************************/
#include "vscs.h"

// read n characters from fd descriptor.

int readn(int fd, void *vptr, int n)
{
	int nleft;
	int nread;
	char *ptr = (char*)vptr;
	nleft = n;
	while(nleft > 0)
	{
		if((nread = read(fd, ptr, nleft)) < 0)
		{
			if(errno == EINTR)
				nread = 0; /* call read() again */
			else
				return ERR;
		}else if(nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft); 
}
