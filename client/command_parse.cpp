/*************************************************************************
	> File Name: command_parse.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 07:56:06 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "command_parse.h"
#include  "transmit_file.h"
#include  "client.h"
#include  <termios.h>
#include  <curses.h>

extern bool stop;
extern bool status_query;
pthread_t thread;
//exclusive access to the download array.
extern pthread_mutex_t download_mutex;
extern unordered_map<string, record> download_array; // all download files
//exclusive access to the upload array.
extern pthread_mutex_t upload_mutex;
extern unordered_map<string, record> upload_array; // all upload files.
extern char ip_address[MAXLINE]; // the ip address of the client.

void command_parse(SSL *ssl, char *command_line)
{
	char command[MAXLINE], arg[MAXLINE];
	char message[MAXLINE];
	int n = sscanf(command_line, "%s%s", command, arg);
	if(strcmp(command, "signup") == 0) // signup a new user.
	{
		if(n != 2)
		{
			printf("wrong command\n");
			return;
		}
		char password[MAXLINE];
		char retype[MAXLINE];
		//close the echo
		struct termios init_settings,new_settings;
		tcgetattr(fileno(stdin), &init_settings);
		new_settings = init_settings;
		new_settings.c_lflag &= ~ECHO;
		if(tcsetattr(fileno(stdin), TCSAFLUSH, &new_settings) != 0)
	    {
			printf("could not set termios attributes\n");
			return;
	    }
		while(true)
		{
			printf("New password:");
			fgets(password,MAXLINE, stdin);
			printf("\nRetry new password:");
			fgets(retype, MAXLINE, stdin);
			if(strcmp(password, retype) == 0)
				break;
			printf("\nSorry, passwords do not match\n");
		}
		printf("\n");
		tcsetattr(fileno(stdin), TCSANOW, &init_settings);
		int len = strlen(password);
		if(password[len - 1] == '\n')
			password[len - 1] = '\0';
		snprintf(message, MAXLINE,"signup %s %s", arg, password);
		int k = strlen(message);
		if(SSL_write(ssl, message, k) != k)
		{
			printf("command_parse: ssl_writen error");
			return;
		}
		k = SSL_read(ssl, message, MAXLINE);
		if(k < 0)
		{
			printf("command_parse: SSL_read error");
			return;
		}
		message[k] = '\0';
		printf("%s\n", message);
	}
	else if(strcmp(command, "signin") == 0) // user login 
	{
		if(n != 2)
		{
			printf("wrong command\n");
			return;
		}
		char password[MAXLINE];
	    //cancel echo
		struct termios init_settings,new_settings;
		tcgetattr(fileno(stdin), &init_settings);
		new_settings = init_settings;
		new_settings.c_lflag &= ~ECHO;
		if(tcsetattr(fileno(stdin), TCSAFLUSH, &new_settings) != 0)
		{
			printf("could not set termios attributes\n");
			return;
		}
		printf("Password: ");
		fgets(password, MAXLINE, stdin);
		printf("\n");
		tcsetattr(fileno(stdin), TCSANOW, &init_settings);
		int len = strlen(password);
		if(password[len - 1] == '\n')
			password[len - 1] = '\0';
		snprintf(message, MAXLINE, "signin %s %s", arg, password);
		SSL_write(ssl, message, strlen(message));
		int k = SSL_read(ssl, message, MAXLINE);
		if(k < 0)
		{
			printf("SSL_read error");
			return;
		}
		message[k] = '\0';
		printf("%s", message);
	}
	else if(strcmp(command, "signout") == 0) // signout
	{
		if(n != 1)
		{
			printf("wrong command\n");
			return;
		}
		SSL_write(ssl, "signout", strlen("signout"));
		stop = true;
	}
	else if(strcmp(command, "delete") == 0) // delete a file
	{
		if(n != 2)
		{
			printf("wrong command\n");
			return;
		}
		int k = strlen(command_line);
		if(ssl_writen(ssl, command_line, k) != k)
		{
			printf("ssl_write error.\n");
			return;
		}
		k = SSL_read(ssl, message, MAXLINE);
		if(k < 0)
		{
			printf("SSL_read error.\n");
			return;
		}
		message[k] = '\0';
		printf("%s", message);
	}
	else if(strcmp(command, "ls") == 0) // list all the files of the user.
	{
		if(n != 1)
		{
			printf("wrong command\n");
			return;
		}
		SSL_write(ssl, "ls", strlen("ls"));
		int len;
		int k = SSL_read(ssl, message, MAXLINE);
		message[k] = '\0';
		if(strcmp(message,"not signed in\n")== 0)
			printf("not signed in.\n");
		else
		{
			sscanf(message, "%d\n", &len);
			SSL_write(ssl, "OK", strlen("OK"));
			char *p_list = (char *)malloc(len + 1);
			int m;
		    if((m = SSL_read(ssl, p_list, len)) < 0)
			{
				p_list[m] = '\0';
				printf("value: %d %s\n", m,  p_list);
				printf("SSL_readn error.\n");
				return; 
			}
			p_list[len] = '\0';
			printf("%s", p_list);
		}
	}
	else if(strcmp(command, "upload") == 0) // upload a file to the slave server
	{
		// commandline format: upload ip_address file_name file_size
		if(n != 2)
		{
			printf("wrong command\n");
			return;
		}
		// add the file to be uploaded to the upload array.
		struct stat64 buf;
		memset(&buf, 0, sizeof(buf));
		int file = open(arg,FILE_MODE);
		fstat64(file, &buf);
		if(file < 0)
		{
			printf("file %s not exist\n", arg);
			return;
		}
		snprintf(message, MAXLINE, "upload %s %s %lld",ip_address, arg, (long long)buf.st_size);
		int len = strlen(message);

		if(ssl_writen(ssl, message, len) != len)
		{
			printf("ssl_writen error\n");
			return;
		}
		int k = SSL_read(ssl, message, MAXLINE);
		if(k < 0)
		{
			printf("SSL_read error\n");
			return;
		}
		message[k] = '\0';
		char temp[MAXLINE];
		snprintf(temp, MAXLINE, "file %s already exists.\n", arg);
		if(strcmp(temp, message) != 0)
		{
			pthread_mutex_lock(&upload_mutex);
			upload_array.insert(make_pair(arg, record(buf.st_size)));
			pthread_mutex_unlock(&upload_mutex);
		}
		close(file);
		printf("%s", message);
	}
	else if(strcmp(command, "download") == 0) // download a file from the slave server.
	{
		if(n != 2)
		{
			printf("wrong command\n");
			return;
		}
		//add the file to be downloaded to the download array.
		pthread_mutex_lock(&download_mutex);
		download_array.insert(make_pair(arg, record()));
		pthread_mutex_unlock(&download_mutex);
		snprintf(message, MAXLINE, "download %s %s", ip_address, arg);
		int len = strlen(message);

		if(ssl_writen(ssl, message, len) != len)
		{
			printf("ssl_writen error\n");
			return;
		}
		
		int k = SSL_read(ssl, message, MAXLINE);
		if(k < 0)
		{
			printf("SSL_read error\n");
			return;
		}
		message[k] = '\0';
		printf("%s", message);
	}
	else if(strcmp(command, "status") == 0)
	{
		if(n != 1)
		{
			printf("wrong command\n");
			return;
		}
		status_query = true;
		pthread_create(&thread, NULL, status_thread, NULL);
		struct termios newt, oldt;
		// save terminal settings
		tcgetattr(0, &oldt);
		// init new settings
		newt = oldt;
		//change settings
		newt.c_lflag &= ~(ICANON | ECHO);
		//apply settings
		tcsetattr(0, TCSANOW, &newt);
		getchar();
		tcsetattr(0, TCSANOW, &oldt);
		status_query = false;
	}
	else 
		printf("command not found\n");
}
