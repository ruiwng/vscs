/*************************************************************************
	> File Name: connected_slaves.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 27 Jul 2015 04:17:46 PM CST
 ************************************************************************/
/*
 * record all slave servers connected to the master server.
 * the operations include insert a connection ,delete a connection
 * traversal all connections
 */
#ifndef   CONNECTED_SLAVES_H
#define   CONNECTED_SLAVES_H

#include  "vscs.h"

struct connection
{
	int sockfd;
	string address;
	SSL *ssl;
};

class connected_slaves
{
private:
	struct info
	{
		int sockfd;
		SSL *ssl;
		info(int s, SSL *ss):sockfd(s), ssl(ss){}
	};
private:
	//record all the connected slaves. key stand for the ip address of slave server.
	//connresponding to the SSL* to it.
	unordered_map<string, info> slaves;  
	pthread_mutex_t slave_mutex; // used for exclusive access to slaves.
	unordered_map<string, info>::iterator current_pos;
public:
	// initialize the the mutex
	connected_slaves(){pthread_mutex_init(&slave_mutex, NULL);}
	~connected_slaves();
	// add a slave connection
	void add_a_connection(const char *ip_address, const int sockfd, SSL *ssl);
	// fetch all the ip address of connections.
	vector<string> get_all_connections();
	// judge whether there is at least on connection or not.
	bool is_empty();
	// judge whether ip address exists in the slaves.
	SSL *is_exist(const char *ip_address);
	// initialize the current_pos to the beginning of connections.
	void init() {current_pos = slaves.begin(); }
	connection get_a_connection();
};

#endif  //CONNECTED_SLAVES_H
