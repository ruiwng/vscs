/*************************************************************************
	> File Name: master_config.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Thu 16 Jul 2015 02:48:07 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "master_config.h"

/*
 * configuration of the following arguments.
 * redis_address : the ip address of the redis database server.
 * redis_port : the port of the redis database server.
 * slave_array : all the slave servers ready to connect to the master server.
 * slave_port : listen to the master to establish a connection to transform upload/download/delete file command from the client.
 * master_port : listen to the clients waiting for them to establish connection.
 * master_status_port : listen to the request to fetch the status of the cluster's running status.
 * slave_listen_port : listen to the request to fetch the status of the current slave's running status.
 * ssl_certificate : a certificate file used to initialize the SSL connection.
 * ssl_key : a file used to generate private key for SSL connection.
 * client_directory : the full directory of the executable client, the file was used to generate a SHA1 check sum to verify the client
 * connected to the server.
 */
int master_configure(char *redis_address, char *redis_port,
		vector<string> &slave_array, char *slave_port, char *master_port, 
		char *slave_status_port, char *master_status_port, char *slave_listen_port, char *ssl_certificate,
		char *ssl_key, char *client_directory)
{
	// initalize all the stuff.
	*redis_address = '\0';
	*redis_port = '\0';
	*slave_port = '\0';
	*master_port = '\0';
	*slave_status_port = '\0';
	*master_status_port = '\0';
	*slave_listen_port = '\0';
	*ssl_certificate = '\0';
	*ssl_key = '\0';
	*client_directory = '\0';
	// open  the master configuration file.
	FILE *f_config = fopen(master_config_file, "r");
	if(f_config == NULL)
	{
		log_msg("master_configure: %s not exists", master_config_file);
		return -1;
	}

	// begin reading the maste configuration file.
	char str[MAXLINE];
	while(fgets(str, MAXLINE, f_config) != NULL)
	{
		char name[MAXLINE], conf[MAXLINE];
		int n=sscanf(str, "%s %s", name, conf);
		if(n == 0) // blank line
			continue;
		else if(name[0] == '#')// comment line
			continue;
		else if(n == 1) //wrong configuration
		{
			log_msg("master_configure: wrong configuration");
			return -1;
		}
		else if(strcmp(name, "REDIS_ADDRESS") == 0) //configuration of the address of the redis database 
		{
			if(*redis_address != '\0')
			{
				log_msg("master_configure: REDIS_ADDRESS duplicate configured");
		 		return -1;
			}
			strcpy(redis_address, conf);
		}
		else if(strcmp(name, "REDIS_PORT") == 0)// configuration of the address of the redis port
		{
			if(*redis_port != '\0')
			{
				log_msg("master_configure: REDIS_PORT duplicate configured");
				return -1;
			}
			strcpy(redis_port, conf);
		}
		else if(strcmp(name, "SLAVE_ADDRESS") == 0)// configuration of the address of slave address
			slave_array.push_back(conf);
		else if(strcmp(name, "SLAVE_PORT") == 0)// configuration of the slave port
		{
			if(*slave_port != '\0')
			{
				log_msg("master_configure: SLAVE_PORT duplicate configured");
				return -1;
			}
			strcpy(slave_port, conf);
		}
		else if(strcmp(name, "MASTER_PORT") == 0)// configuration of the master port
		{
			if(*master_port != '\0')
			{
				log_msg("master_configure: MASTER_PORT duplicate configured");
		 		return -1;
			}
			strcpy(master_port, conf);
		}
		else if(strcmp(name, "SLAVE_STATUS_PORT") == 0) // configuration of the slave status port
		{
			if(*slave_status_port != '\0')
			{
				log_msg("master_configure: SLAVE_STATUS_PORT duplicate configured");
		 		return -1;
			}
			strcpy(slave_status_port, conf);
		}
		else if(strcmp(name, "MASTER_STATUS_PORT") == 0) // configuration of the master status port
		{
			if(*master_status_port != '\0')
			{
			   log_msg("master_configure: MASTER_STATUS_PORT duplicate configured");
			   return -1;
			}
			strcpy(master_status_port, conf);
		}
		else if(strcmp(name, "SLAVE_LISTEN_PORT") == 0) // configuration of the waiting for the connection to be a slave.
		{
			if(*slave_listen_port != '\0')
			{
				log_msg("master_configure: SLAVE_LISTEN_PORT duplicate configured");
				return -1;
			}
			strcpy(slave_listen_port, conf);
		}
		else if(strcmp(name, "SSL_CERTIFICATE") == 0) //certificate file of SSL
		{
			if(*ssl_certificate != '\0')
			{
				log_msg("master_configure: SSL_CERTIFICATE duplicate configured");
		 		return -1;
			}
			strcpy(ssl_certificate, conf);
		}
		else if(strcmp(name, "SSL_KEY") == 0) // private key of SSL
		{
			if(*ssl_key != '\0')
			{
		 		log_msg("master_configure: SSL_KEY duplicate configured");
				return -1; 
			}
			strcpy(ssl_key, conf);
		}
		else if(strcmp(name, "CLIENT_DIRECTORY") == 0) // the full directory to store the executable client.
		{
			if(*client_directory != '\0')
			{
				log_msg("master_configure: CLIENT_DIRECTORY duplicate configured");
				return -1;
			}
			strcpy(client_directory, conf);
		}
		else // unknown configuration
		{
			log_msg("master_configure: unknown configure argument");
			return -1;
		}
	}

	// judge whether there exists arguments not configured.
	if(*redis_address == '\0' || *redis_port == '\0' || slave_array.empty() || *slave_port == '\0'
			|| *master_port == '\0' || *slave_status_port == '\0' || *master_status_port == '\0'
			|| *slave_listen_port == '\0' || *ssl_certificate == '\0' 
			|| *ssl_key == '\0' || *client_directory == '\0')
	{
		log_msg("master_configure: unconfigured argument exist");
		return -1;
	}
	return 0;
}

