/*************************************************************************
	> File Name: vscs.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 11 Jul 2015 03:19:31 PM CST
 ************************************************************************/
#ifndef  VSCS_H
#define  VSCS_H

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
#include  <netdb.h>    /* inclusion for getaddrinfo */
#include  <fcntl.h>  /* for open and fcntl */

//for ssl
#include  <openssl/rsa.h>
#include  <openssl/crypto.h>
#include  <openssl/x509.h>
#include  <openssl/pem.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

#include  <string>
// for hash set.
#include  <unordered_map>
#include  <unordered_set>

using namespace std;

#define   MAXLINE    4096  /* maximum length of a line */
#define   MAXBUF     4096  /* maximum length of buffer */

#define   USER_PREFIX  "user_"  /* the prefix of the user key */
#define   SLAVE_PREFIX "slave_" /* the prefix of the slave server */

const char *const master_config_file = "/etc/vscs_master.conf"; // the configure file of the master server
const char *const slave_config_file = "/etc/vscs_slave.conf"; // the configure file of the slave server
const char *const client_config_file = "/etc/vscs_client.conf"; // the configure file of the client


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

/* Nonfatal error related to a system call. */
void err_ret(const char *fmt, ...);

/* Fatal error related to a system call. */
void err_sys(const char *fmt, ...);

/* Nonfatal error unrelated to a system call */
void err_cont(int error, const char *fmt, ...);

/* Fatal error unrelated to a system call. */
void err_exit(int error, const char *fmt, ...);

/* Fatal error related to a system call. */
void err_dump(const char *fmt, ...);

/* Nonfatal error unrelated to a system call. */
void err_msg(const char *fmt, ...);

/* Fatal error unrelated to a system call. */
void err_quit(const char *fmt, ...);

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

// daemonize the process
void daemonize(const char *cmd);

// create a connection to "address"
// if success, return the socket connected to the remote address, otherwise return -1.
int client_connect(const char *address, const char *port);

// listen to "port"
// if success, return the socket listening to the port, otherwise return -1.
int server_listen(const char *port);

// in case that read was interrupted by signal.
int read_s(int fd, void *buf, int count);

//return null if fail, certificate indicates the certificate file.
//key refer to the private key.
SSL_CTX *ssl_init(const char *certificate, const char *key);

// the server's SSL, if success return SSL*, otherwise return NULL
SSL* ssl_server(SSL_CTX *ctx, int sockfd);

// the client's SSL, if success return SSL*, otherwise return NULL
SSL* ssl_client(SSL_CTX *ctx, int sockfd);

// read n characters from SSL*.
int ssl_readn(SSL *, void *vptr, int n);

// write n characters to SSL*.
int ssl_writen(SSL *, const void *vptr, int n);
#endif /* VSCS_H */
