/*************************************************************************
	> File Name: setnonblocking.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 17 Aug 2015 10:06:26 AM CST
 ************************************************************************/
#include  "vscs.h"

int setnonblocking(int sockfd)
{
	if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1)
		return -1;
	return 0;
}
