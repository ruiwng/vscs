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
#include  <unordered_map>
#include  <string>
using namespace std;

char redis_address[MAXLINE];// address of the redis database server
char redis_port[MAXLINE];// port of the redis database server
int sockdb; // the socket descriptor connected to the redis database server.

vector<string> slave_array; //all the slave servers
char slave_port[MAXLINE];// port of the slave server
char master_port[MAXLINE];// port listening to the clients. 

char slave_status_port[MAXLINE];// status port of the slave server.
char master_status_port[MAXLINE]; // status port of the master server.

char ssl_certificate[MAXLINE]; // certificate file of ssl
char ssl_key[MAXLINE]; // private key of ssl

unordered_map<string, SSL*> connected_slaves; // all the connected slave servers versus their file descriptor.
unordered_map<int, SSL*> connected_clients; // all the clients connected to the server.
unordered_map<string, SSL*>::iterator current_iterator; // the very iterator points the the slave server that will be used.
msg_queue all_msgs;

bool stop = false;   // whether stop the execution or not.
// handle all the messages in the msg queue.
void *msg_handler_thread(void *arg)
{
	SSL *ssl;
	char message[MAXLINE];
	while(!stop)
	{
	   //get a message from the message queue.
	   all_msgs.pop_msg(ssl, message);
	   command_parse(ssl, message);
	}
	return NULL;	
}

// query status of all the servers.
void status_query(int sockfd)
{
	sockaddr_in servaddr;
	socklen_t len = sizeof(servaddr);
	memset(&servaddr, 0, len);
	getsockname(sockfd, (sockaddr*)&servaddr, &len);
	char addr[MAXLINE+1];
	inet_ntop(AF_INET, (void *)&servaddr.sin_addr, addr, MAXLINE);
	char str[MAXLINE+1];
	snprintf(str, MAXLINE, "master server: %s\n", addr);
	writen(sockfd, str, strlen(str));

	//traverse all the clients.
	for(unordered_map<int, SSL*>::iterator iter = connected_clients.begin();
			iter != connected_clients.end(); ++iter)
	{
		sockaddr_in clientaddr;
		len = sizeof(clientaddr);
		memset(&clientaddr, 0, len);
		getpeername(sockfd, (sockaddr*)&clientaddr, &len);
		inet_ntop(AF_INET, (void *)&clientaddr.sin_addr, addr, MAXLINE);
		char str[MAXLINE+1];
		snprintf(str, MAXLINE,"client %s connected\n", addr);
		writen(sockfd, str, strlen(str));
	}
	//traverse all the slave servers.
	for(unordered_map<string, SSL*>::iterator iter = connected_slaves.begin();
			iter != connected_slaves.end(); ++iter)
	{
		int slave_socket = client_connect(iter->first.c_str(), slave_status_port);
		if(slave_socket == -1)
		{
			log_msg("can't accesss status port of %s", iter->first.c_str());
			continue;
		}
		char temp[MAXLINE + 1];
		snprintf(temp, MAXLINE + 1,"slave %s\n",iter->first.c_str());
		writen(sockfd, temp, strlen(temp));
		int n;
		while(true)
		{
			n = read_s(slave_socket, temp, MAXLINE);
			if(n <= 0)
				break;
			temp[n] ='\0';
			writen(sockfd, temp,n);
		}
	}
	close(sockfd);  // close the status socket.
}

int main(int argc, char *argv[])
{
	if(argc != 2 || (strcmp(argv[1], "start") != 0 && strcmp(argv[1], "status") != 0))
	{
		printf("Usage: %s <start/status>\n", argv[0]);
		return -1;
	}
	sleep_us(500000);
	//read the configure file to get redis adress, redis port, slave server array,
	//port of the slave server, port of the master server, status port of slave server
	//status port of master server, certificate of ssl and private key of ssl.
	if(master_configure(redis_address, redis_port, slave_array, slave_port,
				master_port,  slave_status_port, master_status_port, ssl_certificate,
 				ssl_key ) == -1) 
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "master server configure");
	if(strcmp(argv[1], "status") == 0)
	{
		cluster_status(master_status_port);
		return 0;
	}
	
	printf("%-60s[\033[;32mOK\033[0m]\n", "master server configure");
	sleep_us(500000);
	// initialize the SSL
	SSL_CTX *ctx_server = ssl_server_init(ssl_certificate, ssl_key);
	if(ctx_server == NULL)
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "SSL server initialize");
	SSL_CTX *ctx_client = ssl_client_init();
	if(ctx_client == NULL)
		err_quit("%-60s[\033[;31mFAILED\033[0m]", "SSL client initialize");
	printf("%-60s[\033[;32mOK\033[0m]\n", "SSL initialize");
	
	// daemonize the master server.
	daemonize("vscs_master");
	//connect to the redis server.
	if((sockdb = client_connect(redis_address, redis_port)) == -1)
		log_quit("connect to the redis server unsuccessfully");
	log_msg("connect to the redis server successfully");
	int len = slave_array.size();
	// connect to all the slave slave servers.
	for(int i=0; i < len; ++i)
	{
		int sockfd = client_connect(slave_array[i].c_str(), slave_port);
		if(sockfd == -1)
		{
			log_msg("connect to slave server %s:%s", slave_array[i].c_str(), slave_port);
			continue;
		}
		SSL * ssl_temp = ssl_client(ctx_client, sockfd); // establish ssl connection to the slave server.

		if(ssl_temp == NULL)
		{
			log_msg("ssl_client to slave server %s error", slave_array[i].c_str());
			continue;
		}
		//add the current connection to connected slave servers.
		connected_slaves.insert(make_pair(slave_array[i], ssl_temp));

		log_msg("connect to the slave server %s", slave_array[i].c_str());
	}
	if(connected_slaves.empty())
	     log_quit("no slave server available");
	current_iterator = connected_slaves.begin();
	// listen to the clients' connection
	int listenfd = server_listen(master_port);
	if(listenfd == -1)
		log_quit("listen the %s port unsuccessfully", master_port);
	log_msg( "listen to %s port successfully", master_port);
	// listen to status  port
	int statusfd = server_listen(master_status_port);
	if(statusfd == -1)
	{
		log_quit("listen to %s status port unsuccessfully", master_status_port);
	}
	log_msg("listen to %s status port", master_status_port);
	
	// create the message handler thread to handler new message.
	pthread_t thread;
	int k = pthread_create(&thread, NULL, msg_handler_thread, NULL);
	if(k != 0)
	     log_quit("msg_handler_thread create unsuccessfully");
	else 
		 log_msg("msg_handler_thread create successfully");

	fd_set rset, allset;
	int maxfd = listenfd;
	if(maxfd < statusfd)//get the maximum file descriptor
		maxfd = statusfd;

	FD_ZERO(&allset); // initialize fd_set
	FD_SET(listenfd, &allset);
	FD_SET(statusfd, &allset);

	while(!stop)
	{
		rset = allset; // structure assignment 
		int  nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if(nready < 0)
		{
			if(errno == EINTR)
				continue;
			else
				log_sys("select error");
		}
		if(FD_ISSET(listenfd, &rset)) //a new client connected
		{
			// accept the request from the client
			sockaddr_in client_addr;
			socklen_t len = sizeof(client_addr);
			memset(&client_addr, 0, len);
			char dst[MAXLINE];// the address of the conenected client.
			int clientfd = accept(listenfd, (sockaddr*)&client_addr,&len);
            if(clientfd < 0)
			{
	 			log_ret("accept error");
	 			continue; 
			}
			if(inet_ntop(AF_INET,&client_addr.sin_addr.s_addr, dst, MAXLINE) != NULL)
				log_msg("client %s was connected", dst);
			SSL *ss = ssl_server(ctx_server, clientfd);
			int x = SSL_read(ss, dst, MAXLINE);
			if(x < 0)
				log_sys("SSL_read error");
			dst[x] = '\0';
			log_msg("%s", dst);
			if(ss != NULL)
			{
	 			connected_clients.insert(make_pair(clientfd, ss));
				FD_SET(clientfd, &allset); // add the new connection to the set.
				if(clientfd > maxfd)
 	  				maxfd = clientfd; 
			}
			else 
				log_msg("ssl_server error");
			--nready;
		}
		if(nready == 0)
			continue;
		if(FD_ISSET(statusfd, &rset)) // the status of all the servers.
		{
			//accept the request from the master server.
			int masterfd = accept(statusfd, NULL, NULL);
			status_query(masterfd);
			--nready;
		}
		if(nready == 0)
			continue;

		// traverse all the connected clients to see whether they are ready.	
		for(unordered_map<int, SSL *>::iterator iter = connected_clients.begin();
				iter != connected_clients.end();)
		{
			if(FD_ISSET(iter->first, &rset))
			{
				char  message[MAXLINE+1];
				int n;
				while((n = SSL_read(iter->second, message, MAXLINE)) < 0)
				{
					if(n < 0 && errno == EINTR)
						continue;
				}
				if(n < 0)
					n = 0;
				message[n] = '\0';
				log_msg("%s", message);
				all_msgs.push_msg(iter->second, message);// push the message to the message queue.
				// the connection was terminted. so close the file descriptor.
				if(n == 0 || strcmp(message, "signout") == 0)
				{
					SSL_shutdown(iter->second);
					close(iter->first);
					FD_CLR(iter->first, &allset);
					unordered_map<int, SSL *>::iterator iter_temp = iter++;
 	  				connected_clients.erase(iter_temp);
				}
				else
					++iter;
				// no ready clients anymore
				if(--nready == 0)
 	  				break;  
			}
			else
				++iter;
		}
	}
	close(listenfd);
	close(statusfd);
	SSL_CTX_free(ctx_server);
	SSL_CTX_free(ctx_client);
	exit(0);
}

