/*************************************************************************
	> File Name: server_listen.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 17 Jul 2015 03:59:44 PM CST
 ************************************************************************/
#include  "csd.h"

//listen to the port 
int server_listen(const char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC; /* allow IPv4 and IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int n;
	if((n = getaddrinfo(NULL, port, &hints, &result)) != 0)
	{
		log_msg("server_listen: getaddrinfo %s", gai_strerror(n));
		return -1;
	}

	int sockfd;
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{
		sockfd = socket(rp->ai_family , rp->ai_socktype, rp->ai_protocol);
		if(sockfd == -1)
			continue;
		if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; /* success */
		close(sockfd);
	}
	
	freeaddrinfo(result);
	if(rp == NULL)
	{
		log_msg("server_listen: listen to %s unsuccessfully", port);
		return -1;
	}

	return sockfd;
}	
