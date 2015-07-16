/*************************************************************************
	> File Name: csd.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 11 Jul 2015 03:19:31 PM CST
 ************************************************************************/
#ifndef  CSD_H
#define  CSD_H

#include  <stdio.h> /* inclusion  for convenience */ 
#include  <unistd.h>/* inclusion for convenience */
#include  <stdlib.h> /* inclusion for convenience */
#include  <string.h> /* inclusion for string operation */
#include  <errno.h>  /* for errno */
#include  <sys/signal.h> /* for signal */
#include  <sys/types.h>
#include  <sys/socket.h> /* for socket */
#include  <sys/stat.h>
#include  <netinet/in.h>
#include  <arpa/inet.h> /* for inet_pton */
#include  <pthread.h>   /* for pthread_create */

#define   MAXLINE    4096  /* maximum length of a line */
#define   MAXBUF     4096  /* maximum length of buffer */

#define   USER_PREFIX  "user_"  /* the prefix of the user key */
#define   SLAVE_PREFIX "slave_" /* the prefix of the slave server */

const char *const REDIS_IP = "127.0.0.1"; // ip address of the redis
const int REDIS_PORT = 6379;// port of the redis

const int SLAVE_PORT = 8989; // the listen port of the slave server

const int CLIENT_DOWNLOAD_PORT = 8990; // the download port of the client 
const int CLIENT_UPLOAD_PORT = 8991; // the updoad port of the client

const char *const STORE_PATH = "/slave_store"; // the directory to store files

// Default file access permissions for new file
#define   FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

// Default directory access permossions for new directory
#define   DIR_MODE (S_IXUSR | S_IXGRP | S_IXOTH | FILE_MODE)

#define    LISTEN_COUNT          2048 // the count of listen
#define     OK                   0 // definition of success

#define     ERR                  -1 //definition of error
#define     USER_ALREADY_EXIST    0X01 // user already exist
#define     ERR_USER_ADD          0X02 // error in add new user
#define     USER_NOT_EXIST        0X03 // user not exist
#define     WRONG_PASSWD          0X04 // wrong passwd

#define     FILE_ALREADY_EXIST    0X05 //file already exist
#define     FILE_NOT_EXIST        0X06 // file not exist
#define     ERR_RESTORE_FILE      0X07 // file restore error

// file operation
#define     REMOVE_FILE           0X08 // remove a file from the slave server 
#define     DOWNLOAD_FILE         0X09 // download a file from the slave server
#define     UPLOAD_FILE           0X0A  // upload a file to the slave server

// initialize the log
void log_open(const char *name, int option, int facility);

/* nonfatal error related to a system call */
void log_ret(const char *fmt, ...);

/* Fatal error related to a system call. */
void log_sys(const char *fmt, ...);

/* Nonfatal error unrelated to a system call. */
void log_msg(const char *fmt, ...);

/* Fatal error unrelated to a system call */
void log_quit(const char *fmt, ...);

/* Fatal error related to a system call */
void log_exit(int error, const char *fmt, ...);

/* read n characters from fd descriptor */
int readn(int fd, void *vptr, int n);

/* write n characters to fd descriptor */
int writen(int fd, const void *vptr, int n);

void daemonize(const char *cmd);

#endif /* CSD_H */
