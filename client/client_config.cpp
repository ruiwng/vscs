/*************************************************************************
	> File Name: client_config.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 06:51:17 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "client_config.h"

int client_configure(char *master_port, char *transmit_port, char *ssl_certificate,
		char *ssl_key)
{
	*master_port = '\0';
	*transmit_port = '\0';
	*ssl_certificate = '\0';
	*ssl_key = '\0';

	FILE *f_config = fopen(client_config_file, "r");
	if(f_config == NULL)
	{
		printf("client_configure: %s not exists\n", client_config_file);
		return -1;
	}
	char str[MAXLINE];
	while(fgets(str, MAXLINE, f_config) != NULL)
	{
		char name[MAXLINE], conf[MAXLINE];
		int n = sscanf(str, "%s %s", name, conf);
		if(n == 0) //blank line
			continue;
		else if(name[0] == '#') // comment line
			continue;
		else if(n == 1) // wrong configuration
		{
			printf("client_configure: wrong configuration\n");
			return -1;
		}
		else if(strcmp(name, "MASTER_PORT") == 0) // master port
		{
			if(*master_port != '\0')
			{
				printf("client_configure: MASTER_PORT duplicate configured\n");
				return -1; 
			}
			strcpy(master_port, conf);
		}
		else if(strcmp(name, "TRANSMIT_PORT") == 0) //transmit port
		{
			if(*transmit_port != '\0')
			{
				printf("client_configure: TRANSMIT_PORT duplicate configured\n");
				return -1; 
			}
			strcpy(master_port, conf);
		}
		else if(strcmp(name, "SSL_CERTIFICATE") == 0) //ssl certificate file
		{
			if(*ssl_certificate != '\0')
			{
				printf("client_configure: SSL_CERTIFICATE duplicate configured\n");
				return -1;
			}
			strcpy(ssl_certificate, conf);
		}
		else if(strcmp(name, "SSL_KEY") == 0) //ssl private key file
		{
			if(*ssl_key != '\0')
			{
				printf("client_configure: SSL_KEY duplicate configured\n");
				return -1;
			}
			strcpy(ssl_key, conf);
		}
		else
		{
			printf("client_configure: unknown configure argument\n");
			return -1;
		}
	}
	if(*master_port == '\0' || *transmit_port == '\0' || *ssl_certificate == '\0'
			|| *ssl_key == '\0')
	{
		log_msg("unconfigured argument exists");
		return -1;
	}
	return 0;
}
