/*************************************************************************
	> File Name: command_parse.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 08:47:18 PM CST
 ************************************************************************/
#ifndef   COMMAND_PARSE_H
#define   COMMAND_PARSE_H

// delete a file
void delete_file(char *command_line);
// download files from the slave server
void *download_thread(void *);
// upload files to the slave server
void *upload_thread(void *);
//parse the command
void command_parse(const char *command_line);

#endif    // COMMAND_PARSE_H
