/*************************************************************************
	> File Name: command_parse.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 08:47:18 PM CST
 ************************************************************************/
#ifndef   COMMAND_PARSE_H
#define   COMMAND_PARSE_H

//delete a file
void delete_file(char *command_line);

//the client download file from the master server.
void *download_thread(void *command_line);

//the client upload file to the slave server.
void *upload_thread(void *command_line);
//parse the command
void command_parse(const char *command_line);

#endif    // COMMAND_PARSE_H
