/*************************************************************************
	> File Name: init.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 15 Jul 2015 03:59:29 PM CST
 ************************************************************************/

#include  "csd.h"
#include  <string>
#include  <vector>
using namespace std;

//initialize the database server
int init_db()
{
	//initialize the connection
	int sock_db;
	if((sock_db = socket(AF_IENT, SOCK_STREAM, 0)) < 0)
	{
		log_sys("init_db: socket error");
		return -1;
	}
	sockaddr_in db_addr;
	memset(&db_addr, 0, sizeof(db_addr));
	db_addr.sin_family = AF_INET;
	db_addr.sin_port = htons(REDIS_PORT);
	if(inet_pton(AF_INET, REDIS_IP, &db_addr.sin_addr) < 0)
	{
		log_sys("init_db: inet_pton error");
		return -1;
	}
	// try to connect to the database 
	if(connect(sock_db, (struct sockaddr*)&db_addr, sizeof(db_addr)) < 0)
	{
		log_sys("init_db: connect error");
		return -1;
	}
	log_msg("init database successfully");
	return sock_db;
}

//initilize the slave server
void init_slave(const vector<string> &slave_array)
{
	int len = slave_array.size();
	// traverse all the ip address and try to connect them.
	for(int i=0; i < len; ++i)
	{
		//initialize connection 
		int sock_db;
		if((sock_db = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			log_ret("init_slave: socket error");
			continue;
		}
		sockaddr_in slave_addr;
		memset(&slave_addr, 0, sizeof(slave_addr));
		slave_addr.sin_family = AF_INET;
		slave_addr.sin_port = htons(SLAVE_PORT);
		
		if(inet_pton(AF_INET, slave_array[i].c_str(), &slave_addr.sin_addr) < 0)
		{
			log_ret("init_slave: inet_pton error");
			continue;
		}
		// try to connnect to the current ip address.
		if(connect(sock_slave, (struct sockaddr*) &slave_addr, sizeof(slave_addr)) < 0)
		{
			log_ret("init_slave: connect error");
			continue;
		}
		log_msg("init_slave: slave %s connected successfully", slave_array.c_str());
	}
}
