/*************************************************************************
	> File Name: connected_slaves.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 27 Jul 2015 04:17:35 PM CST
 ************************************************************************/
#include  "connected_slaves.h"

// shutdown all the ssls, close all the socket descriptors, the free all the ssls.
connected_slaves::~connected_slaves()
{
	// lock the slaves.
	pthread_mutex_lock(&slave_mutex);
	
	// shutdown the ssl, close the socket descriptor, and free ssl.
	for(unordered_map<string, info>::iterator iter = slaves.begin(); iter != slaves.end();
			++iter)
	{
		SSL_shutdown(iter->second.ssl);
		close(iter->second.sockfd);
		SSL_free(iter->second.ssl);
	}

	// unlock the slaves.
	pthread_mutex_unlock(&slave_mutex);
	
	// destroy the mutex.
	pthread_mutex_destroy(&slave_mutex);
}

// add a new connection to the slaves.
void connected_slaves::add_a_connection(const char *ip_address, int sockfd, SSL *ssl)
{
	// lock the slaves.
	pthread_mutex_lock(&slave_mutex);

	// add a new connection to the slaves
	slaves.insert(make_pair(ip_address, info(sockfd, ssl)));

	// unlock the slaves.
	pthread_mutex_unlock(&slave_mutex);
}

// fetch all the ip address of connections.
vector<string> connected_slaves::get_all_connections()
{
	vector<string> result;

	// lock the slaves.
	pthread_mutex_lock(&slave_mutex);

	// traverse the connections and fetch ip addresses
	for(unordered_map<string, info>::iterator iter = slaves.begin(); iter != slaves.end();
			++iter)
		result.push_back(iter->first);

	// unlock the slaves.
	pthread_mutex_unlock(&slave_mutex);
	return result;
}

// judge whether there is at least one connection or not.
bool connected_slaves::is_empty() 
{
	bool empty;
	// lock the slaves.
	pthread_mutex_lock(&slave_mutex);

	empty = slaves.empty();

	// unlock the slaves.
	pthread_mutex_unlock(&slave_mutex);

	return empty;
}

// judge whether ip address exists in the slaves.
SSL *connected_slaves::is_exist(const char *ip_address)
{
	SSL *result;
	// lock the slaves.
	pthread_mutex_lock(&slave_mutex);

	unordered_map<string, info>::iterator iter = slaves.find(ip_address);
	if(iter == slaves.end())
		result = NULL;
	else
		result = iter->second.ssl;
	// unlock the slaves.
	pthread_mutex_unlock(&slave_mutex);

	return result;
}

// get a slave connection
connection connected_slaves::get_a_connection()
{
	connection result;
	// lock the slaves.
	pthread_mutex_lock(&slave_mutex);

	result.ssl = current_pos->second.ssl;
	result.sockfd = current_pos->second.sockfd;
	result.address = current_pos->first;
	if(++ current_pos == slaves.end())
		current_pos = slaves.begin();
	if(slaves.size() != 1)
		result.next_address = current_pos->first;
	else
		result.next_address = "";

	// unlock the slaves.
	pthread_mutex_unlock(&slave_mutex);
	return result;
}

