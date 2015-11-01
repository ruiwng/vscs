/*************************************************************************
	> File Name: slave_server.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 07:52:21 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "master_connect.h"
#include  "slave_config.h"
#include  "file_backup.h"

bool stop = false; //whether stop the execution of the slave server or not.

SSL_CTX *ctx_server;
SSL_CTX *ctx_client;

char client_transmit_port[MAXLINE];  // the file transmit port to the client

char slave_port[MAXLINE]; // port of the slave server.
char slave_status_port[MAXLINE]; // status port of the slave server.
char backup_port[MAXLINE]; // used for backup.
char slave_listen_port[MAXLINE]; // wait fr a connection to be a slave.

char ssl_certificate[MAXLINE]; // certificate file of ssl
char ssl_key[MAXLINE]; // private key of ssl

char store_dir[MAXLINE]; // directory to store files.

// download-related stuff.
pthread_mutex_t download_mutex;
unordered_set<string> download_array;

// upload-related stuff.
pthread_mutex_t upload_mutex;
unordered_set<string> upload_array;

// backup-related stuff.
pthread_mutex_t backup_mutex;
unordered_set<string> backup_array;

sigset_t mask;

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

	 //send all backup connections.
	 pthread_mutex_lock(&backup_mutex);
	 for(unordered_set<string>::iterator iter = backup_array.begin();
			 iter != backup_array.end(); ++iter)
	 {
		 char str[MAXLINE + 1];
		 snprintf(str, MAXLINE, "   %s backing up\n", iter->c_str());
		 SSL_write(ssl, str, strlen(str));
	 }
	 if(empty && !backup_array.empty())
		 empty = false;
	 pthread_mutex_unlock(&backup_mutex);

	 //if no download connection and upload connection, (nil) was transmitted.
	 SSL_write(ssl, "    (nil)\n", strlen("    (nil)\n"));

	 close(sockfd);
	 SSL_free(ssl);
}

/*
 * a thread dealing with signals
 */
void *signal_thread(void *arg)
{
	int err, signo;

	for( ; ; )
	{
		err = sigwait(&mask, &signo);
		if(err != 0)
			log_quit("sigwait failed: %s", strerror(err));
		switch(signo)
		{
			case SIGTERM:
				stop = true;
				break;
			default:
				stop = true;
				log_quit("unexpected signal %d", signo);
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	// read the configure to configure client download port, client upload port, slave port
	// slave status port ssl certificate, and ssl key.
	sleep_us(500000);
	if(slave_configure(slave_port, slave_status_port, backup_port, client_transmit_port,
				 slave_listen_port, ssl_certificate, ssl_key, store_dir) == -1)
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "slave server configure");
	printf("%-60s[\033[;32mOK\033[0m]\n", "slave server configure");

	// initialize the SSL
	sleep_us(500000);
	ctx_server = ssl_server_init(ssl_certificate, ssl_key);
	if(ctx_server == NULL)
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "SSL server initialize");
	ctx_client = ssl_client_init();
	if(ctx_client == NULL)
		log_quit("%-60s[\033[;31mFAILED\033[0m]", "SSL client initialize");

	printf("%-60s[\033[;32mOK\033[0m]\n", "SSL initialize");

	//iniitialize the mutex.
	pthread_mutex_init(&download_mutex, NULL);
	pthread_mutex_init(&upload_mutex, NULL);
	pthread_mutex_init(&backup_mutex, NULL);

	//daemoize the process
	daemonize("vscs_slave");

	/*
	 * handle the signal.
	 */
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	// ignore SIGPIPE signal
	if(sigaction(SIGPIPE, &sa, NULL) < 0)
		log_sys("sigaction failed");

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	int err;
	if(( err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
		log_sys("pthread_sigmask failed");
	
	pthread_t thread;
	/* 
	 * connect to all the masters given in the argv
	 */
	for( int i = 1; i < argc; ++ i)
	{
		int master = client_connect(argv[i], slave_listen_port);
		if(master >= 0)
		{
			int *temp = (int *)malloc(sizeof(int));
			*temp = master;
			pthread_create(&thread, NULL, master_connect_thread, static_cast<void *>(temp));
		}
		else
			log_msg("cannot connect to master server %s", argv[i]);
	}

	/*
	 * listen to the master's connection
	 */
	int listenfd = server_listen(slave_port);
	if(listenfd == -1)
		log_quit("cannot listen the %s port", slave_port);
	log_msg("listen to %s port successfully", slave_port);

	/*
	 * listen to the master's status connection.
	 */
	int statusfd = server_listen(slave_status_port);
	if(statusfd == -1)
		log_quit("cannot listen to %s status port", slave_status_port);
	log_msg("listen to %s status port successfully", slave_status_port);
	
	/*
	 * listen to the backup connection.
	 */
	int backupfd = server_listen(backup_port);
	if(backupfd == -1)
		log_quit("cannot listen to %s backup port", backup_port);
	log_msg("listen to %s backup port successfully", backup_port);

	fd_set rset, allset;
	int maxfd = listenfd;
	if(maxfd < statusfd) // get the maximum file descriptor
		maxfd = statusfd;
	if(maxfd < backupfd)
		maxfd = backupfd;

	FD_ZERO(&allset); // initialize fd_set
	FD_SET(listenfd, &allset);
	FD_SET(statusfd, &allset);
	FD_SET(backupfd, &allset);

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
			{
				int *temp = (int *)malloc(sizeof(int));
				*temp = masterfd;
			    pthread_create(&thread, NULL, master_connect_thread, static_cast<void *>(temp));
			}
			-- nready;
		}
		if(nready == 0)
			continue;
		if(FD_ISSET(statusfd, &rset)) // query status from the master server
		{
			// accept the query request from the master server.
			int masterfd = accept(statusfd, NULL, NULL);
			if(masterfd < 0)
				log_msg("slave server accept master status query error");
			else
				query_status(masterfd);
			-- nready;
		}
		if(nready == 0)
			continue;
		if(FD_ISSET(backupfd, &rset)) // a backup connection occurs
		{
			// accept the backup request from the slave server.
			int slavefd = accept(backupfd, NULL, NULL);
			if(slavefd < 0)
				log_msg("slave server accept backup error");
			else
			{
				int *temp = (int *)malloc(sizeof(int));
				*temp = slavefd;
	 			pthread_create(&thread, NULL, backup_thread, (void *)temp);
			}
		}
	}
	close(listenfd);
	close(statusfd);
	close(backupfd);
	
	SSL_CTX_free(ctx_server);
	SSL_CTX_free(ctx_client);

	pthread_mutex_destroy(&download_mutex);
	pthread_mutex_destroy(&upload_mutex);
	pthread_mutex_destroy(&backup_mutex);
	return 0;
}

