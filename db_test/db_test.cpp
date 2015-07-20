/*************************************************************************
	> File Name: db_test.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 12 Jul 2015 09:44:23 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "client_info.h"

int main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in db_addr;
	memset(&db_addr,0,sizeof(db_addr));
	db_addr.sin_family = AF_INET;
	db_addr.sin_port = htons(REDIS_PORT);
	inet_pton(AF_INET,REDIS_IP, &db_addr.sin_addr);
	connect(sockfd, (sockaddr*)&db_addr, sizeof(db_addr));

	client_info *p_client = new client_info(sockfd, "ruiwng1234");
	char *q = p_client->show_filelist();
	printf("%s\n",q);
	free(q);

	p_client->add_file("Advanced_Programming_in_the_UNIX_Environment",
			530301, "10.66.27.243");
	p_client->add_file("Beginning_Linux_programming",30302, "10.66.27.232");
	p_client->add_file("UNIX_Network_Programming", 50031, "10.66.27.223");
	q = p_client->show_filelist();
	printf("%s\n", q);
	free(q);

	p_client->delete_file("Beginning_Linux_programming");
	q = p_client->show_filelist();
	printf("%s\n", q);

	char buf[MAXLINE];
	p_client->query_file_storage("UNIX_Network_Programming", buf);
	printf("%s\n", buf);
	
	delete p_client;

	return 0;
}
