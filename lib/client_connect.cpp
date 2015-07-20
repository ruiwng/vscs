/*************************************************************************
	> File Name: client_connect.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 17 Jul 2015 03:50:00 PM CST
 ************************************************************************/
#include  "vscs.h"

int client_connect(const char *address, const char *port)
{
	//initialize the connection 
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
	hints.ai_socktype = SOCK_STREAM;

	int n;
	if((n = getaddrinfo(address, port, &hints, &result)) != 0)
	{
		log_msg("client_connect: getaddrinfo error %s", gai_strerror(n));
		return -1;
	}
	int sockfd;
	// traverse all the available address
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sockfd == -1)
			continue;
		if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break; /* success */
		
		close(sockfd);
	}

	freeaddrinfo(result);

	if(rp == NULL)
	{
		// can't connect to the current address
		return -1;
	}

	return sockfd;
}
