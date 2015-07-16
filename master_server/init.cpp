/*************************************************************************
	> File Name: init.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 15 Jul 2015 03:59:29 PM CST
 ************************************************************************/

#include  "csd.h"
#include  "init.h"

//initialize the database server
//redis_ip stands for the ip address of the redis datababse server.
//redis_port stands forthe port of the redis database server.
//the return value is the file descriptor connect to the redis.
int init_db(const char *redis_ip, const char *redis_port)
{
	//initialize the connection
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	
	/* get the address of the database server */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	
	int n;
	if((n = getaddrinfo(redis_ip, redis_port, &hints, &result)) != 0)
		log_quit("init_db: getaddrinfo error %s",gai_strerror(s) );

	int sock_db;
	// traverse all the available address
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{
		sock_db = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock_fd == -1)
			continue;

		if(connect(sock_fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break; /* Success */

		close(sock_fd);
	}
	if(rp == NULL)
		//can't connect to the database server.
		log_quit("init_db: connect to the database server unsuccessfully");

	freeaddrinfo(result);

	log_msg("init_db: connect to the datababse successfully");
	
	return sock_db;
}

//initilize the slave server
//slave_array stands for all the available slave servers.
int init_slave(const vector<string> &slave_array, const char *slave_port, unordered_map<string, int> &available_slave)
{
	int len = slave_array.size();
	// traverse all the ip address and try to connect them.
	int number = 0;
	for(int i=0; i < len; ++i)
	{
		struct addrinfo hints;
		struct addrinfo *result, *rp;
		
		/* obtain address according to the slave ip address */
		memset(hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC; /* Allow both IPv4 and IPv6 */
		hints.ai_socktype = SOCK_STREAM; 
		
		int n;
		if((n = getaddrinfo(slave_array[i].c_str(), slave_port, &hints, &result)) != 0)
		{
			log_msg("init_slave: getaddrinfo: %s connect to %s", gai_strerror(n), slave_array.c_str());
			continue;
		}

		int sock_slave;
		for(rp = result; rp != NULL; rp = rp->ai_next)
		{
			sock_slave=socket(rp->ai_family , rp->ai_socktype, rp->ai_protocol);
			if(sock_slave == -1)
				continue;
			if(connect(sock_slave, rp->ai_addr, rp->ai_addrlen) != -1)
				break; // connect successfully
			close(sock_slave);
		}
		//current slave is available
		if(rp != NULL)
		{
			++number;
			available_slave.insert(make_pair(slave_array[i],sock_slave));
		}
		else
			log_msg("init_slave: %s is unavailable",string[i].c_str());
		freeaddrinfo(result);
	}
	return number;
}

// initialize the master server 
// listen to the client.
int init_master(const char *master_port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC; /* allow IPv4 and IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	int n;
	if(( n = getaddrinfo(NULL, master_port, &hints, &result)) != 0)
		log_quit("init_master: getaddrinfo %s", gai_strerror(n));

	int sock_master;
	for(rp = rp->result; rp != NULL; rp = rp->ai_next)
	{
		sock_master = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock_master == -1)
			continue;
		if(bind(sock_master, rp->ai_addr, rp->ai_addrlen) == 0)
			break; /* success */
		close(sock_master);
	}
	// init master failed
	if(rp == NULL)
		log_quit("init_master: init master unsuccessfully");
	freeaddrinfo(result);
	
	return sock_master;
}


