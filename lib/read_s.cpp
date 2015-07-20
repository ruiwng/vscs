/*************************************************************************
	> File Name: read_s.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 17 Jul 2015 04:44:29 PM CST
 ************************************************************************/
#include  "vscs.h"

int read_s(int fd, void *buf, int count)
{
	int n;
	while((n = read(fd, buf, count)) < 0)
	{
		if(errno == EINTR)
			continue;
		else
			return -1;
	}
	return n;
}
