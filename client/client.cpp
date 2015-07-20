/*************************************************************************
	> File Name: client.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 07:32:30 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "client_config.h"
#include  "command_parse.h"
#include  "client.h"
#include  "transmit_file.h"

char master_port[MAXLINE]; // the port of the master server.
char transmit_port[MAXLINE]; // the port of the file transmit.
char ssl_certificate[MAXLINE]; // the ssl certificate file.
char ssl_key[MAXLINE]; // the ssl key file.
SSL_CTX *ctx;
bool stop = false; // whether to stop the execution or not.

// exculsive access to the download array.
pthread_mutex_t download_mutex; 
// all download files
unordered_map<string, record> download_array;
// exclusive access to the upload array.
pthread_mutex_t upload_mutex;
// all upload files
unordered_map<string, record> upload_array;

// accept transmit request from the slave server.
void *transmit_thread(void *arg)
{
	int transmitfd = (int)arg;
	pthread_t thread;
	// accept the download/upload request from the slave server.
	while(!stop)
	{
		int slave_fd = accept(transmitfd, NULL, NULL);
		if(slave_fd < 0)
		{
			err_ret("transmit_thread: accept error");
			continue;
		}
		SSL *ssl = ssl_server(ctx, slave_fd);
		if(ssl == NULL)
		{
			close(slave_fd);
			err_msg("transmit_thread: ssl_server error");
			continue;
		}
		char message[MAXLINE + 1];
		int n = SSL_read(ssl, message, MAXLINE);
		message[n] = '\0';
		char command[MAXLINE], file_name[MAXLINE];
		int file_size;
		int k = sscanf(message,"%s%s%d", command, file_name, &file_size);

		bool exist = false;
		if(strcmp(command, "upload") == 0)
		{
			if(k != 2)
			{
				close(slave_fd);
				SSL_free(ssl);
				err_msg("transmit_thread: wrong command");
				continue;
			}
			// find whether the upload job exist.
			pthread_mutex_lock(&upload_mutex);
			unordered_map<string, record>::iterator iter = upload_array.find(file_name);
		    if(iter != upload_array.end())
				exist = true;
			pthread_mutex_unlock(&upload_mutex);
			if(exist)
			{
				transmit_arg *down_arg = new transmit_arg(slave_fd, ssl, iter, file_name);
				pthread_create(&thread, NULL, download_thread, (void *)down_arg);
			}
			else
			{
				close(slave_fd);
				SSL_free(ssl);
				err_msg("transmit_thread:  upload file not found.");  
			}

		}
		else if(strcmp(command, "download") == 0)
		{
			if(k != 3)
			{
				close(slave_fd);  
				SSL_free(ssl);
				err_msg("transmit_thread: wrong command");
				continue;
			}
			// find whether the download job exist.
			pthread_mutex_lock(&download_mutex);
			unordered_map<string, record>::iterator iter = download_array.find(file_name);
			if(iter != download_array.end())
			{
				iter->second.sum =  file_size; 
				exist = true; 
			}
			pthread_mutex_unlock(&download_mutex);
			if(exist)
			{
				transmit_arg *up_arg = new transmit_arg(slave_fd, ssl, iter, file_name);
				pthread_create(&thread, NULL, upload_thread, (void *)up_arg);
			}
		}
		else
		{  
			// unrecoginized command.
		    close(slave_fd);
			SSL_free(ssl);
			err_msg("transmit_thread: wrong command");
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("usage : %s <ipaddress>\n", argv[0]);
		return -1;
	}

	// configure the client.
	if(client_configure(master_port, transmit_port, ssl_certificate, ssl_key) < 0)
	{
		printf("client configured failed");
		return -1;
	}
	
	// initialize the SSL
	ctx = ssl_init(ssl_certificate, ssl_key);
	if(ctx == NULL)
	{
		printf("SSL initialize unsuccessfully");
		return -1;
	}
	printf("SSL initialize successfully");

	// connect to the master server.
	int masterfd = client_connect(argv[1], master_port);
	if(masterfd == -1)
	{
		printf("cannot connect to master server: %s\n", argv[1]);
		return -1;
	}
	printf("connected to the master server: %s\n", argv[1]);
	
	SSL *ssl = ssl_client(ctx, masterfd);
	if(ssl == NULL)
	{
		printf("ssl_client error\n");
		return -1;
	}
	
	int listenfd = server_listen(transmit_port);
	if(listenfd == -1)
	{
		printf("listen to %s unsuccessfully\n", transmit_port);
		return -1;
	}
	pthread_mutex_init(&download_mutex, NULL);
	pthread_mutex_init(&upload_mutex, NULL);
	char command_line[MAXLINE];

	while(!stop)
	{
		printf("%s:%s>", argv[0], master_port);
		//read the command inputed by the user.
		fgets(command_line, MAXLINE, stdin);
		int len = strlen(command_line);
		if(command_line[len-1] == '\n')
			command_line[len-1] = '\0';
		command_parse(ssl, command_line);
	}
	
	SSL_shutdown(ssl); // send SSL/TLS close_notify */
	close(masterfd);

	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}
