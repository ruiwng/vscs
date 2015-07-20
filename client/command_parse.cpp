/*************************************************************************
	> File Name: command_parse.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 07:56:06 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "command_parse.h"
#include  "client.h"
#include  <termios.h>
#include  <curses.h>

extern bool stop;

//exclusive access to the download array.
extern pthread_mutex_t download_mutex;
extern unordered_map<string, record> download_array; // all download files
//exclusive access to the upload array.
extern pthread_mutex_t upload_mutex;
extern unordered_map<string, record> upload_array; // all upload files.

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
		cbreak();
		noecho(); //cancel echo
		while(true)
		{
			printf("New password: ");
			scanf("%s", password);
			printf("Retry new password: ");
			scanf("%s", retype);
			if(strcmp(password, retype) == 0)
				break;
			printf("Sorry, passwords do not match\n");
		}
		echo();
		nocbreak();
		snprintf(message, MAXLINE,"signup %s %s", arg, password);
		SSL_write(ssl, message, strlen(message));
		int k = SSL_read(ssl, message, MAXLINE);
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
		cbreak();
		noecho(); //cancel echo
		printf("Password: ");
		scanf("%s", password);
		echo();
		nocbreak();
		snprintf(message, MAXLINE, "signin %s %s", arg, password);
		SSL_write(ssl, message, strlen(message));
		int k = SSL_read(ssl, message, MAXLINE);
		message[k] = '\0';
		printf("%s\n", message);
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
		if(strcmp(message,"Please login.\n")== 0)
			printf("Please login.\n");
		else
		{
			sscanf(message, "%d", &len);
			SSL_write(ssl, "OK", strlen("OK"));
			char *p_list = (char *)malloc(len + 1);
			if(SSL_readn(ssl, p_list, len) != len)
			{
				printf("SSL_readn error.\n");
				return;
			}
			p_list[len] = '\0';
			printf("%s\n", p_list);
		}
	}
	else if(strcmp(command, "upload") == 0) // upload a file to the slave server
	{
		if(n != 2)
		{
			printf("wrong command\n");
			return;
		}
		int file = open(arg, O_RDONLY);
		if(file == -1)
		{
			printf("file not exists\n");
			return;
		}
		struct stat buf;
		fstat(file,&buf);
		// add the file to be uploaded to the upload array.
		pthread_mutex_lock(&upload_mutex);
		upload_array.insert(make_pair(arg, record(buf.st_size)));
		pthread_mutex_unlock(&upload_mutex);
		close(file);
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
	}
	else if(strcmp(command, "status") == 0)
	{
		if(n != 1)
		{
			printf("wrong command\n");
			return;
		}

	}
	else 
		printf("command not found\n");
}
