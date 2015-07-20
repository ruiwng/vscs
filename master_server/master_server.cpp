/*************************************************************************
	> File Name: master_server.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 18 Jul 2015 10:54:33 AM CST
 ************************************************************************/
#include  "vscs.h"
#include  "master_config.h"
#include  "msg_queue.h"
#include  <unordered_map>
#include  <string>
using namespace std;

char redis_address[MAXLINE];// address of the redis database server
char redis_port[MAXLINE];// port of the redis database server

vector<string> slave_array; //all the slave servers
char slave_port[MAXLINE];// port of the slave server
char master_port[MAXLINE];// port listening to the clients. 

char slave_status_port[MAXLINE];// status port of the slave server.
char master_status_port[MAXLINE]; // status port of the master server.

char ssl_certificate[MAXLINE]; // certificate file of ssl
char ssl_key[MAXLINE]; // private key of ssl

unordered_map<string, SSL*> connected_slaves; // all the connected slave servers versus their file descriptor.
unordered_map<int, SSL*> connected_clients; // all the clients connected to the server.
msg_queue all_msgs;

bool stop = false;   // whether stop the execution or not.
// handle all the messages in the msg queue.
void *msg_handler_thread(void *arg)
{
	return NULL;	
}

// query status of all the servers.
void status_query(int sockfd)
{
	sockaddr_in servaddr;
	socklen_t len = sizeof(servaddr);
	getsockname(sockfd, (sockaddr*)&servaddr, &len);
	char addr[MAXLINE+1];
	inet_ntop(AF_INET, (void *)&servaddr, addr, MAXLINE);
	char str[MAXLINE+1];
	snprintf(str, MAXLINE, "master server: %s\n", addr);
	writen(sockfd, str, strlen(str));

	//traverse all the clients.
	for(unordered_map<int, SSL*>::iterator iter = connected_clients.begin();
			iter != connected_clients.end(); ++iter)
	{
		sockaddr_in clientaddr;
		len = sizeof(clientaddr);
		getpeername(sockfd, (sockaddr*)&clientaddr, &len);
		inet_ntop(AF_INET, (void *)&clientaddr, addr, MAXLINE);
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
		printf("slave %s\n",iter->first.c_str());
		char temp[MAXLINE + 1];
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
	//read the configure file to get redis adress, redis port, slave server array,
	//port of the slave server, port of the master server, status port of slave server
	//status port of master server, certificate of ssl and private key of ssl.
	if(master_configure(redis_address, redis_port, slave_array, slave_port,
				master_port, slave_status_port, master_status_port, ssl_certificate,
				ssl_key) == -1)
		log_quit("master server configure failed");

	log_msg("master server configure successfully");

	// initialize the SSL
	SSL_CTX *ctx = ssl_init(ssl_certificate, ssl_key);
	if(ctx == NULL)
		log_quit("SSL initialize failed");
	log_msg("SSL initialize successfully");

	int sock_db;
	//connect to the redis server.
	if((sock_db = client_connect(redis_address, redis_port)) == -1)
		log_quit("cannot connect to the redis server");
	log_msg("connect to the redis server successfully");


	int len = slave_array.size();
	// connect to all the slave slave servers.
	for(int i=0; i < len; ++i)
	{
		int sockfd = client_connect(slave_array[i].c_str(), slave_port);
		if(sockfd == -1)
			log_msg("cannot connect to slave server %s:%s", slave_array[i].c_str(), slave_port);
		SSL * ssl; // establish ssl connection to the slave server.
		ssl = SSL_new(ctx);

		if(ssl == NULL)
		{
			log_msg("SSL_new to slave server %s error", slave_array[i].c_str());
			continue;
		}
		SSL_set_fd(ssl, sockfd);
		int ret = SSL_connect(ssl);

		if(ret == -1)
		{
			log_msg("SSL_connect to slave server %s error", slave_array[i].c_str());
			continue;
		}
		//add the current connection to connected slave servers.
		connected_slaves.insert(make_pair(slave_array[i], ssl));

		log_msg("connect to the slave server %s successfully", slave_array[i].c_str());
	}
	if(!slave_array.empty())
		log_quit("no slave server available");

	// listen to the clients' connection
	int listenfd = server_listen(master_port);
	if(listenfd == -1)
		log_quit("cannot listen the %s port", master_port);

	// listen to status  port
	int statusfd = server_listen(master_status_port);
	if(statusfd == -1)
		log_quit("cannot listen to %s status port", master_status_port);

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
				log_quit("select error");
		}
		if(FD_ISSET(listenfd, &rset)) //a new client connected
		{
			// accept the request from the client
			int clientfd = accept(listenfd, NULL ,NULL);
            
			SSL *ss = ssl_server(ctx, clientfd);
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
		
		for(unordered_map<int, SSL *>::iterator iter = connected_clients.begin();
				iter != connected_clients.end();)
		{
			if(FD_ISSET(iter->first, &rset))
			{
				char  message[MAXLINE+1];
				int n = SSL_read(iter->second, message, MAXLINE);
				message[n] = '\0';
				all_msgs.push_msg(iter->second, message);// push the message to the message queue.
				// the connection was terminted. so close the file descriptor.
				if(n == 0)
				{
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
	exit(0);
}

