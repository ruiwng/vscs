/*************************************************************************
	> File Name: command_parse.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Tue 21 Jul 2015 11:29:13 AM CST
 ************************************************************************/
#ifndef   COMMAND_PARSE_H
#define   COMMAND_PARSE_H

// parse the command from the client.
void command_parse(int sockfd, SSL *ssl,const char*command_line);

#endif
