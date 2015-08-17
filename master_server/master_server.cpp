/*************************************************************************
	> File Name: master_server.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 18 Jul 2015 10:54:33 AM CST
 ************************************************************************/
#include  "vscs.h"
#include  "master_config.h"
#include  "msg_queue.h"
#include  "command_parse.h"
#include  "cluster_status.h"
#include  "connected_slaves.h"
#include  <unordered_map>
#include  <string>
using namespace std;

#define    MAX_EVENTS               10

char redis_address[MAXLINE];// address of the redis database server
char redis_port[MAXLINE];// port of the redis database server
int sockdb; // the socket descriptor connected to the redis database server.

vector<string> slave_array; //all the slave servers
char slave_port[MAXLINE];// port of the slave server
char master_port[MAXLINE];// port listening to the clients. 

char slave_status_port[MAXLINE];// status port of the slave server.
char master_status_port[MAXLINE]; // status port of the master server.
char slave_listen_port[MAXLINE]; // wait for a connection to be a slave server.

char ssl_certificate[MAXLINE]; // certificate file of ssl
char ssl_key[MAXLINE]; // private key of ssl

/*
 * SSL-related stuff.
 */
SSL_CTX *ctx_server;  // run as a server.
SSL_CTX *ctx_client;  // run as a client.

/*
 * client-connection-related stuff.
 */
unordered_map<int, SSL*> connected_clients;

msg_queue all_msgs;
connected_slaves all_slaves;

sigset_t mask; 

bool stop = false;   // whether stop the execution or not.

// handle all the messages in the msg queue.
void *msg_handler_thread(void *arg)
{
	int sockfd;
	SSL *ssl;
	char message[MAXLINE];
	while(!stop)
	{
	   //get a message from the message queue.
	   all_msgs.pop_msg(sockfd, ssl, message);
	   log_msg("message: %s", message);
	   command_parse(sockfd, ssl, message);
	}
	return NULL;	
}

/*
 * Deal with signals.
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

/*
 * query status of all the servers.
 */
void  *status_query_thread(void *arg)
{
	int sockfd = (int)arg;
	SSL *ssl = ssl_server(ctx_server, sockfd);
	sockaddr_in servaddr;
	socklen_t len = sizeof(servaddr);
	memset(&servaddr, 0, len);
	getsockname(sockfd, (sockaddr*)&servaddr, &len);
	char addr[MAXLINE+1];
	inet_ntop(AF_INET, (void *)&servaddr.sin_addr.s_addr, addr, MAXLINE);
	char str[MAXLINE+1];
	snprintf(str, MAXLINE, "master server: %s\n", addr);
	SSL_write(ssl, str, strlen(str));

	/*
	 * traverse all the clients.
	 */
	for(unordered_map<int, SSL*>::iterator iter = connected_clients.begin();
			iter != connected_clients.end(); ++iter)
	{
		sockaddr_in clientaddr;
		len = sizeof(clientaddr);
		memset(&clientaddr, 0, len);
		getpeername(iter->first, (sockaddr*)&clientaddr, &len);
		inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, addr, MAXLINE);
		char str[MAXLINE+1];
		snprintf(str, MAXLINE,"client %s connected\n", addr);
		SSL_write(ssl, str, strlen(str));
	}

	vector<string> slaves = all_slaves.get_all_connections();
	/*
	 * traverse all the slave servers.
	 */
	for(vector<string>::iterator iter = slaves.begin();
			iter != slaves.end(); ++iter)
	{
		// connect to the slave servers' status port and fetch information.
		int slave_socket = client_connect(iter->c_str(), slave_status_port);
		if(slave_socket == -1)
		{
			log_msg("can't accesss status port of %s", iter->c_str());
			continue;
		}
		SSL *ssl_slave = ssl_client(ctx_client, slave_socket);
		if(ssl_slave == NULL)
		{
			log_msg("can't ssl_client to %s", iter->c_str());
			continue;
		}
		char temp[MAXLINE + 1];
		snprintf(temp, MAXLINE + 1,"slave %s\n",iter->c_str());
		SSL_write(ssl, temp, strlen(temp));
		int n;
		while(true)
		{
			n = SSL_read(ssl_slave, temp, MAXLINE);
			if(n <= 0)
				break;
			temp[n] ='\0';
			SSL_write(ssl, temp,n);
		}
		SSL_shutdown(ssl_slave);
		close(slave_socket);
		SSL_free(ssl_slave);
	}
	/*
	 * terminate the connection to the query socket.
	 */
	SSL_shutdown(ssl);
	close(sockfd);  // close the status socket.
	SSL_free(ssl);

	return NULL;
}

int main(int argc, char *argv[])
{
	if(argc != 2 || (strcmp(argv[1], "start") != 0 && strcmp(argv[1], "status") != 0))
	{
		printf("Usage: %s <start/status>\n", argv[0]);
		return -1;
	}
	sleep_us(500000);

	char client_directory[MAXLINE];
	//read the configure file to get redis adress, redis port, slave server array,
	//port of the slave server, port of the master server, status port of slave server
	//status port of master server, certificate of ssl and private key of ssl.
	if(master_configure(redis_address, redis_port, slave_array, slave_port,
 				master_port,  slave_status_port, master_status_port, slave_listen_port, ssl_certificate,
 	 			ssl_key, client_directory) == -1) 
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "master server configure");

	printf("%-60s[\033[;32mOK\033[0m]\n", "master server configure");
	sleep_us(500000);

	/* 
	 * initialize the SSL
	 */
	ctx_server = ssl_server_init(ssl_certificate, ssl_key);
	if(ctx_server == NULL)
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "SSL server initialize");
	ctx_client = ssl_client_init();
	if(ctx_client == NULL)
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "SSL client initialize");
	printf("%-60s[\033[;32mOK\033[0m]\n", "SSL initialize");

	// arg[1] == "status" query the status of the server.
	if(strcmp(argv[1], "status") == 0)
	{
		cluster_status();
		return 0;
	}

	/*
	 * daemonize the master server.
	 */
	daemonize("vscs_master");

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
	if((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
		log_sys("pthread_sigmask failed");

	/*
	 * get the SHA1 check sum of the client.
	 */
	char *ver_client = file_verify(client_directory);
	if(ver_client == NULL)
		log_sys("get file %s's SHA1 checksum unsuccessfully");
	/*
	 * connect to the redis database server.
	 */
	if((sockdb = client_connect(redis_address, redis_port)) == -1)
		log_quit("connect to the redis server unsuccessfully");
	log_msg("connect to the redis server successfully");

	/*
	 * connect to all the slave slave servers.
	 */
	int len = slave_array.size();
	for(int i=0; i < len; ++i)
	{
		int sockfd = client_connect(slave_array[i].c_str(), slave_port);
		if(sockfd == -1)
		{
			log_msg("cannot connect to slave server %s:%s", slave_array[i].c_str(), slave_port);
			continue;
		}
		SSL * ssl_temp = ssl_client(ctx_client, sockfd); // establish ssl connection to the slave server.

		if(ssl_temp == NULL)
		{
			log_msg("ssl_client to slave server %s error", slave_array[i].c_str());
			continue;
		}
		//add the current connection to connected slave servers.
		all_slaves.add_a_connection(slave_array[i].c_str(), sockfd, ssl_temp);

		log_msg("connect to the slave server %s", slave_array[i].c_str());
	}
	if(all_slaves.is_empty())
	     log_quit("no slave server available");
	all_slaves.init();

   /*
   * Create the epollfd
   */
   struct epoll_event ev, events[MAX_EVENTS];
   int epollfd = epoll_create(10);
	/*
	 * listen to the clients' connection
	 */
	int listenfd = server_listen(master_port);
	if(listenfd == -1)
		log_quit("listen to the %s port unsuccessfully", master_port);
	log_msg( "listen to %s port successfully", master_port);
	
	/* add listenfd to epoll set*/
	setnonblocking(listenfd);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listenfd;
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
		log_quit("add listenfd to epoll unsuccessfully");
		
	/*
	 * listen to status  port
	 */
	int statusfd = server_listen(master_status_port);
	if(statusfd == -1)
		log_quit("listen to %s status port unsuccessfully", master_status_port);
	log_msg("listen to %s status port successfully", master_status_port);
	
	/* add statusfd to epoll set */
	setnonblocking(statusfd);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = statusfd;
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, statusfd, &ev) == -1)
		log_quit("add statusfd to epoll unsuccessfully");

	
	/*
	 * listen to a new connection to be a slave server.
	 */
	int slave_listenfd = server_listen(slave_listen_port);
	if(slave_listenfd == -1)
		log_quit("listen to %s slave listen port unsuccessfully", slave_listen_port);
	log_msg("listen to %s slave listen port successfully", slave_listen_port);
	
	/* add slave_listenfd to epoll set */
	setnonblocking(slave_listenfd);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = slave_listenfd;
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, slave_listenfd, &ev) == -1)
		log_quit("add slave_listenfd to epoll unsuccessfully");
		
	/*
	 * create the message handler thread to handler new message.
	 */
	pthread_t thread;
	int k = pthread_create(&thread, NULL, msg_handler_thread, NULL);
	if(k != 0)
	     log_quit("msg_handler_thread create unsuccessfully");
	else 
		 log_msg("msg_handler_thread create successfully");

	/*
	 * create the signal handler thread.
	 */
    err = pthread_create(&thread, NULL, signal_thread, NULL);
	if(err != 0)
		log_quit("signal_thread create unsuccessfully");
	else
		log_msg("signal_thread create successfully");

	/*
	 * wait for the connection of listenfd, statusfd and slave_listenfd.
	 */
	while(!stop)
	{
		int  nready = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if(nready < 0)
		{
			if(errno == EINTR)
				continue;
			else
				log_sys("select error");
		}
		for(int i = 0; i < nready; ++i)
		{
			if(events[i].data.fd == listenfd) // a new client connected
			{
				 //accept  the request from the client
				 sockaddr_in client_addr;
				 socklen_t len = sizeof(client_addr);
				 memset(&client_addr, 0, len);
				 char dst[MAXLINE + 1]; // the address of the connected client.
				 int clientfd = accept(listenfd, (sockaddr*)&client_addr, &len);
				 if(clientfd >= 0)
		   	 {
				   if(inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, dst, MAXLINE) != NULL)
					   log_msg("client %s was connected", dst);
				   SSL *ss = ssl_server(ctx_server, clientfd);
				   if(ss != NULL)
				   {
					   int x = SSL_read(ss, dst, MAXLINE);
					   dst[x] = '\0';
					   if(strcmp(dst, ver_client) == 0)
					   {
						    log_msg("SSA1 check sum %s", dst);
						    //verify the client successfully.
						    // add it to the connected clients.
						    connected_clients.insert(make_pair(clientfd, ss));
						    setnonblocking(clientfd);
						    // add the new connection to the set.
						    ev.events = EPOLLIN | EPOLLET;
						    ev.data.fd = clientfd;
						    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) == -1)
						    	log_quit("epoll_ctl: add clientfd unsuccessfully");
						    	
						    const char *temp = "verify client successfully";
						    size_t n = SSL_write(ss, temp, strlen(temp));
						    if(n != strlen(temp))
							      log_msg("SSL_write error");
					    }
					    else
					    {
						     // ver fiy the client unsuccessfully.
						     // terminate the connection to the client. shutdown SSL and close socket descriptor.
						     const char *temp = "verify client unsuccessfully";
						     size_t n = SSL_write(ss, temp, strlen(temp));
						     if(n != strlen(temp))
							      log_msg("SSL_write error");
						     SSL_shutdown(ss);
						     close(clientfd);
						     SSL_free(ss);
					    }
				    }
				    else
					    log_ret("ssl_server error");
			     }
			     else
				      log_ret("accept error");
			}
			else if(events[i].data.fd == statusfd) // the status of the cluster.
			{
				 // accept the request from the master server.
				 int masterfd = accept(statusfd, NULL, NULL);
				 pthread_create(&thread, NULL, status_query_thread, (void *)masterfd);
			}
			else if(events[i].data.fd == slave_listenfd) // a new connection to be a slave server.
			{
				// accept the request from a connection as a slave.
				sockaddr_in slave_addr;
				socklen_t len = sizeof(slave_addr);
				memset(&slave_addr, 0, sizeof(slave_addr));
				int slavefd = accept(slave_listenfd, (sockaddr*)&slave_addr, &len);
				
				// get the ip address of the peer connection.
				char ip_address[MAXLINE];
				inet_ntop(AF_INET, (void *)&slave_addr.sin_addr, ip_address, MAXLINE);
				
				SSL *ssl_temp = ssl_server(ctx_server, slavefd);
				if(ssl_temp == NULL)
			  {
				   log_msg("a new slave server established unsuccessfully");
			  }
			  else
			  {
				  // add a new slave server to the slave connections.
				  all_slaves.add_a_connection(ip_address, slavefd, ssl_temp);
			  }
			}
			else // client is available
			{
					unordered_map<int, SSL *>::iterator iter = connected_clients.find(events[i].data.fd);
					if(iter != connected_clients.end())
					{
						char message[MAXLINE + 1];
						int n;
						while((n = SSL_read(iter->second, message, MAXLINE)) < 0)
						{
								if(n < 0 && errno == EINTR)
									continue;
						}
						if(n < 0)
							n = 0;
						message[n] = '\0';
						all_msgs.push_msg(iter->first, iter->second, message); // push the message to the message queue.
						// the connection was terminated. so close the file descriptor.
						if(n == 0 || strcmp(message, "exit") == 0)
						{
							 epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
							 connected_clients.erase(iter);
						}
					}
					else
						log_msg("can't find the relative SSL handle to  a socket");
			}
		}
	}
	
	close(listenfd);
	close(statusfd);
	close(slave_listenfd);
	close(epollfd);
	
	SSL_CTX_free(ctx_server);
	SSL_CTX_free(ctx_client);
	exit(0);
}

