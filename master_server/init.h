/*************************************************************************
	> File Name: init.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Thu 16 Jul 2015 01:42:50 PM CST
 ************************************************************************/
#ifndef   INIT_H
#define   INIT_H

#include  <string>
#include  <vector>
#include  <unordered_map>
using namespace std;

//initialize the redis database server
int init_db(const char *redis_ip, const char *redis_port);

//initialize the slave server
// the return value is the count of the available slave server
int init_slave(const vector<string> &slave_array ,unordered_map<string,int> &available_slave);

//initialize the master server
//wait for the clients' connections and queries from the administrator
int init_master(const char *master_port, const char *status_port);
#endif   // INIT_H
