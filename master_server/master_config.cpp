/*************************************************************************
	> File Name: master_config.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Thu 16 Jul 2015 02:48:07 PM CST
 ************************************************************************/
#include  "csd.h"
#include  "master_config.h"

void master_configure(char *redis_address, char *redis_port,
		vector<string> &slave_array, char *slave_port, char *master_port, 
		char *slave_status_port, char *master_status_port)
{
	*redis_address = '\0';
	*redis_port = '\0';
	*slave_port = '\0';
	*master_port = '\0';
	*status_port = '\0';
	*slave_status_port = '\0';
	*master_status_port = '\0';
	FILE *f_config = fopen(master_configure, "r");
	if(f_config == NULL)
		log_quit("master_configure: %s not exists", master_configure);

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
			log_quit("master_configure: wrong configuration");
		else if(strcmp(name, "REDIS_ADDRESS") == 0) //configuration of the address of the redis database 
		{
			if(*redis_address != '\0')
				log_quit("master_configure: REDIS_ADDRESS duplicate configured");
			strcpy(redis_address, conf);
		}
		else if(strcmp(name, "REDIS_PORT") == 0)// configuration of the address of the redis port
		{
			if(*redis_port != '\0')
				log_quit("master_configure: REDIS_PORT duplicate configured");
			strcpy(redis_port, conf);
		}
		else if(strcmp(name, "SLAVE_ADDRESS") == 0)// configuration of the address of slave address
			slave_array.push_back(conf);
		else if(strcmp(name, "SLAVE_PORT") == 0)// configuration of the slave port
		{
			if(*slave_port != '\0')
				log_quit("master_configure: SLAVE_PORT duplicate configured");
			strcpy(slave_port, conf);
		}
		else if(strcmp(name, "MASTER_PORT") == 0)// configuration of the master port
		{
			if(*master_port != '\0')
				log_quit("master_configure: MASTER_PORT duplicate configured");
			strcpy(master_port, conf);
		}
		else if(strcmp(name, "SLAVE_STATUS_PORT") == 0) // configuration of the slave status port
		{
			if(*slave_status_port != '\0')
				log_quit("master_configure: SLAVE_STATUS_PORT duplicate configured");
			strcpy(slave_status_port, conf);
		}
		else if(strcmp(name, "MASTER_STATUS_PORT") == 0) // configuration of the master status port
		{
			if(*slave_status_port != '\0')
			   log_quit("master_configure: MASTER_STATUS_PORT duplicate configured");
			strcpy(master_status_port, conf);
		}
		else // unknown configuration
			log_quit("master_configure: unknown configure argument");
	}
}

