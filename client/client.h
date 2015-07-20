/*************************************************************************
	> File Name: client.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 20 Jul 2015 01:35:58 PM CST
 ************************************************************************/
#ifndef   CLIENT_H
#define   CLIENT_H
#include  "vscs.h"

//record structure was used to record the download or upload information of a file.
//sum is the size of the file, current indicates the size that has already downloaded.
//prev is previous size of the current.

struct record
{
	int sum;
	int current;
	int prev;
	record(int s=0, int c=0, int p=0):sum(s),current(c),prev(p){}
};

// the argument that transmit_thread pass to the download/upload thread.
struct transmit_arg
{
	int sockfd;
	SSL *ssl;
	unordered_map<string, record>::iterator iter;
	char file_name[MAXLINE];
	transmit_arg(int s, SSL *ss,unordered_map<string, record>::iterator i, const char *name):sockfd(s),ssl(ss)
		,iter(i)
	{
		strcpy(file_name, name);
	}
};
#endif   // CLIENT_H
