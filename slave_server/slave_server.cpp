/*************************************************************************
	> File Name: slave_server.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 07:52:21 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "master_connect.h"
#include  "slave_config.h"

bool stop = false; //whether stop the execution of the slave server or not.

SSL_CTX *ctx_server;
SSL_CTX *ctx_client;

char client_transmit_port[MAXLINE];  // the file transmit port to the client

char slave_port[MAXLINE]; // port of the slave server.
char slave_status_port[MAXLINE]; // status port of the slave server.

char ssl_certificate[MAXLINE]; // certificate file of ssl
char ssl_key[MAXLINE]; // private key of ssl

char store_dir[MAXLINE]; // directory to store files.

pthread_mutex_t download_mutex;
unordered_set<string> download_array;

pthread_mutex_t upload_mutex;
unordered_set<string> upload_array;

void query_status(int sockfd)
{
	SSL *ssl = ssl_server(ctx_server, sockfd);
	if(ssl == NULL)
	{
		log_msg("query_status: ssl_server error");
		return;
	}
	bool empty = true;
	//send all the download connections.
	 pthread_mutex_lock(&download_mutex);
	 for(unordered_set<string>::iterator iter = download_array.begin();
			 iter != download_array.end(); ++iter)
	 {
		char str[MAXLINE + 1];
		snprintf(str, MAXLINE, "    %s  downloading\n",iter->c_str());
		SSL_write(ssl, str, strlen(str));
	 }
	 if(!download_array.empty())
		 empty = false;
	 pthread_mutex_unlock(&download_mutex);

	 //send all upload connections.
	 pthread_mutex_lock(&upload_mutex);
	 for(unordered_set<string>::iterator iter = upload_array.begin();
			 iter != upload_array.end(); ++iter)
	 {
		char str[MAXLINE + 1];
		snprintf(str, MAXLINE,"     %s uploading\n", iter->c_str());
		SSL_write(ssl, str, strlen(str));
	 }
	 if(empty && !upload_array.empty())
		 empty = false;
	 pthread_mutex_unlock(&upload_mutex);
	 //if no download connection and upload connection, (nil) was transmitted.
	 SSL_write(ssl, "    (nil)", strlen("    (nil)"));

	 close(sockfd);
	 SSL_free(ssl);
}

int main(int argc, char *argv[])
{
	// read the configure to configure client download port, client upload port, slave port
	// slave status port ssl certificate, and ssl key.
	if(slave_configure(slave_port, slave_status_port, client_transmit_port,
				 ssl_certificate, ssl_key, store_dir) == -1)
		log_quit("slave server configure failed");
	log_msg("slave server configure successfully");

	// initialize the SSL
	ctx_server = ssl_server_init(ssl_certificate, ssl_key);
	if(ctx_server == NULL)
		log_quit("SSL server initialize failed");
	ctx_client = ssl_client_init();
	if(ctx_client == NULL)
		log_quit("SSL client initialize failed");

	log_msg("SSL initialize successfully");

	//iniitialize the mutex.
	pthread_mutex_init(&download_mutex, NULL);
	pthread_mutex_init(&upload_mutex, NULL);
	
	//daemoize the process
	daemonize("vscs_slave");
	// listen to the master's connection
	int listenfd = server_listen(slave_port);
	if(listenfd == -1)
		log_quit("cannot listen the %s port", slave_port);
	
	log_msg("listen to %s port successfully", slave_port);
	// listen to the master's status connection.
	int statusfd = server_listen(slave_status_port);
	if(statusfd == -1)
		log_quit("cannot listen to %s status port", slave_status_port);
	
	log_msg("listen to %s status port successfully", slave_status_port);
	
	fd_set rset, allset;
	int maxfd = listenfd;
	if(maxfd < statusfd) // get the maximum file descriptor
		maxfd = statusfd;

	FD_ZERO(&allset); // initialize fd_set
	FD_SET(listenfd, &allset);
	FD_SET(statusfd, &allset);
	
	pthread_t thread;
	while(!stop)
	{
		rset = allset; // structure assignment
		int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if(nready < 0)
		{
			if(errno == EINTR)
				continue;
			else
				log_sys("select error");
		}
		if(FD_ISSET(listenfd, &rset)) // a new master connected
		{
			// accept the request from the master server.
			int masterfd = accept(listenfd, NULL, NULL);
			if(masterfd < 0)
				log_msg("slave server accept master error");
			else
			    pthread_create(&thread, NULL, master_connect_thread, (void *)masterfd);		
		}
		if(FD_ISSET(statusfd, &rset)) // query status from the master server
		{
			// accept the query request from the master server.
			int masterfd = accept(statusfd, NULL, NULL);
			if(masterfd < 0)
				log_msg("slave server accept master status query error");
			else
				query_status(masterfd);
		}
	}
	close(listenfd);
	close(statusfd);

	SSL_CTX_free(ctx_server);
	SSL_CTX_free(ctx_client);
	return 0;
}

