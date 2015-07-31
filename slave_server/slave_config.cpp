/*************************************************************************
	> File Name: slave_config.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 18 Jul 2015 08:51:37 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "slave_config.h"

int slave_configure(char *slave_port, char *slave_status_port, char *backup_port, char *client_transmit_port,
		char *slave_listen_port, char *ssl_certificate, char *ssl_key, char *store_dir)
{
	*slave_port = '\0';
	*slave_status_port = '\0';
	*backup_port = '\0';
	*client_transmit_port = '\0';
	*slave_listen_port = '\0';
	*ssl_certificate = '\0';
	*ssl_key = '\0';
	*store_dir = '\0';

	FILE *f_config = fopen(slave_config_file, "r");
	if(f_config == NULL)
	{
		log_msg("slave_configure: %s not exists", slave_configure);
		return -1;
	}
	char str[MAXLINE];
	while(fgets(str, MAXLINE, f_config) != NULL)
	{
		char name[MAXLINE], conf[MAXLINE];
		int n = sscanf(str, "%s %s", name, conf);
		if( n == 0) //blank line
			continue;
		else if(name[0] == '#') //comment line
			continue;
		else if(n == 1) //wrong configuration
		{
			log_msg("slave_configure: wrong configuration");
			return -1;
		}
		else if(strcmp(name, "SLAVE_PORT") == 0)// slave listen port
		{
			if(*slave_port != '\0')
			{
				log_msg("slave_configure: SLAVE_PORT duplicate configured");
				return -1; 
			}
			strcpy(slave_port, conf);
		}
		else if(strcmp(name, "SLAVE_STATUS_PORT") == 0)// slave status port.
		{
			if(*slave_status_port != '\0')
			{
				log_msg("slave_configure: SLAVE_STATUS_PORT duplicate configured");
				return -1;
			}
			strcpy(slave_status_port, conf);
		}
		else if(strcmp(name, "BACKUP_PORT") == 0) // used for backup
		{
			if(*backup_port != '\0')
			{
				log_msg("slave_configure: BACKUP_PORT duplicate configured");
				return -1;
			}
			strcpy(backup_port, conf);
		}
		else if(strcmp(name, "CLIENT_TRANSMIT_PORT") == 0)//client transmit port
		{
			if(*client_transmit_port != '\0')
			{
				log_msg("slave_configure: SLAVE_DOWNLOAD_PORT duplicate configured");
				 return  - 1;
			}
			strcpy(client_transmit_port, conf);
		}
		else if(strcmp(name, "SLAVE_LISTEN_PORT") == 0) // wait for a connection to be a slave server.
		{
			if(*slave_listen_port != '\0')
			{
				log_msg("slave_configure: SLAVE_LISTEN_PORT duplicate configured");
				return -1;
			}
			strcpy(slave_listen_port, conf);
		}
		else if(strcmp(name, "SSL_CERTIFICATE") == 0) //certificate file of SSL
		{
			if(*ssl_certificate != '\0')
			{
		 		log_msg("slave_ configure: SSL_CERTIFICATE duplicate configured");
				return -1; 
			}
			strcpy(ssl_certificate, conf);
		}
		else if(strcmp(name, "SSL_KEY") == 0) // private key of SSL
		{
			if(*ssl_key != '\0')
			{
				log_msg("slave_configure: SSL_KEY duplicate configured");
				return -1; 
			}
			strcpy(ssl_key, conf);
		}
		else if(strcmp(name, "STORE_DIR") == 0) //directory to store files
		{
			if(*store_dir != '\0')
			{
				log_msg("slave_configure: STORE_DIR duplicate configured");
				return -1;
			}
			strcpy(store_dir, conf);
		}
		else // unknown configuration
		{
			log_msg("slave_configure: unknown configure argument");
			return -1;
		}
	}

	if(*slave_port == '\0' || *slave_status_port == '\0' || *client_transmit_port == '\0' || *backup_port == '\0' || 
			 *ssl_certificate == '\0' ||
			*ssl_key == '\0' || *store_dir == '\0')
	{
		log_msg("unconfigured argument exists");
		return -1;
	}
	return 0;
}

