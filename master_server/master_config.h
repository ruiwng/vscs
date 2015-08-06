/*************************************************************************
	> File Name: master_config.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Thu 16 Jul 2015 03:02:09 PM CST
 ************************************************************************/
#ifndef  MASTER_CONFIG_H
#define  MASTER_CONFIG_H

#include  <vector>
#include  <string>
using namespace std;

//read the configure file, include the address of the redis database,
//the port of the redis database, the ip address of all the slave servers.
//the port of the slave, the port of the master server and the status port of the master server.
//the port waiting for a connection to be a slave. the full directory of the client file.
// if success return zero, otherwise return -1.

int master_configure(char *redis_address, char *redis_port,
		vector<string> &slave_array, char *slave_port, char *master_port,
		char *slave_status_port, char *master_status_port, char *slave_listen_port, char *ssl_certificate,
		char *ssl_key, char *client_directory);

#endif // MASTER_CONFIG_H
