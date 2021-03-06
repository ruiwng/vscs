/*************************************************************************
	> File Name: command_parse.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Tue 21 Jul 2015 11:34:19 AM CST
 ************************************************************************/
#include  "vscs.h"
#include  "command_parse.h"
#include  "client_info.h"
#include  "connected_slaves.h"

extern int sockdb; // the socket descriptor connected to the Redis database server.
unordered_map<SSL *,client_info*> signin_users;// all signin users recorded here.
extern connected_slaves all_slaves; // all the connected slave servers versus their SSL*.

char *p = NULL;
int k = 0;
// parse the command from the client.
void command_parse(int sockfd, SSL *ssl,const char *command_line)
{
	char command[MAXLINE], arg1[MAXLINE], arg2[MAXLINE];
	long long arg3;
	char message[MAXLINE + 1];
	int n = sscanf(command_line, "%s%s%s%lld", command, arg1, arg2, &arg3);

	// whether or not the user has login.
	unordered_map<SSL *,client_info*>::iterator iter = signin_users.find(ssl);
	bool signed_in = (iter != signin_users.end());

	if(strcmp(command,"signup") == 0) // signup a new user
	{
		if(n != 3)
			strcpy(message, "wrong command.\n");
		else
		{
		   int k = user_add(sockdb, arg1, arg2);
		   if(k == USER_ALREADY_EXIST)
			   snprintf(message, MAXLINE, "user %s already exists.\n", arg1);
		   else if(k == ERR_USER_ADD)
			   snprintf(message, MAXLINE, "user %s added unsuccessfully.\n", arg1);
		   else
			   snprintf(message, MAXLINE, "user %s added successfully.\n", arg1);
		}
		int len = strlen(message);
		if(ssl_writen(ssl, message, len) != len)
			log_msg("command_parse: ssl_write error");
	}
	else if(strcmp(command, "signin") == 0) // user login
	{
		if(n != 3)
			strcpy(message, "wrong command.\n");
		else
		{
			int k = user_login(sockdb, arg1, arg2);
			if(k == USER_NOT_EXIST) // the user doesn't exist.
				snprintf(message, MAXLINE, "user %s not exists.\n", arg1);
			else if(k == WRONG_PASSWD) // the password is wrong.
				snprintf(message, MAXLINE, "wrong password.\n");
			else if(signed_in)
				snprintf(message, MAXLINE, "user %s already signin.\n", arg1);
			else
			{
				signin_users[ssl] = new client_info(sockdb, arg1); 
	 			snprintf(message, MAXLINE, "user %s signin successfully.\n", arg1);
			}
		}
		int len = strlen(message);
		if(ssl_writen(ssl, message, len) != len)
			log_msg("command_parse: SSL_write error");
	}
	else if(strcmp(command, "signout") == 0) // signout
	{
		if( n != 1)
		{
			strcpy(message, "wrong command.\n");
			int len = strlen(message);
			if(ssl_writen(ssl, message, len) != len)
				log_msg("command_parse: ssl_writen error");
		}
		else
		{
			if(signed_in)
			{
				snprintf(message, MAXLINE, "user %s signout successfully\n", iter->second->get_clientname());
	 			delete iter->second;
				signin_users.erase(iter);
			}
			else
				strcpy(message, "not signed in.\n");
		}
		int len = strlen(message);
		if(ssl_writen(ssl, message, len) != len)
			log_msg("command_parse: ssl_writen error");
	}
	else if(*command_line == '\0' || strcmp(command, "exit") == 0) //exit
	{
		if(*command_line != '\0' && n != 1)
			strcpy(message, "wrong command\n");
		else
		{
			if(signed_in)
			{
				delete iter->second;
				signin_users.erase(iter);
			}
			SSL_shutdown(ssl);
			close(sockfd);
			SSL_free(ssl);
		}
	}
	else if(strcmp(command, "ls") == 0) //list all the files of the user.
	{
		if(n != 1)
		{
			strcpy(message, "wrong command.\n");
			int len = strlen(message);
			if(ssl_writen(ssl, message, len) != len)
				log_msg("command_parse: SSL_write error");
		}
		else if(!signed_in)
		{
			strcpy(message, "not signed in.\n");
			int len = strlen(message);
			if(ssl_writen(ssl, message, len) != len)
				log_msg("command_parse: SSL_write error");
		}
		else
		{
			char *p = iter->second->show_filelist();
			int k = strlen(p);
			char q[MAXLINE];
			snprintf(q, MAXLINE, "%d\n",k);
			int len = strlen(q) + k;
			char *sendbuf = (char *)malloc(len + 1);
			strcpy(sendbuf, q);
			strcat(sendbuf, p);
			free(p);
			if(ssl_writen(ssl, sendbuf, len) != len)
				log_msg("command_parse: ssl_writen error");
			free(sendbuf);
		}
	}
	else if(strcmp(command, "upload") == 0) // upload a file to the slave user.
	{
		// commandline format: upload(command) ip_address(arg1) file_name(arg2) file_size(arg3)
		if(n != 4)
			strcpy(message, "wrong command.\n");
		else if(!signed_in)
			strcpy(message, "not signed in.\n");
		else if(iter->second->is_file_exist(arg2))
			snprintf(message, MAXLINE, "file %s already exists.\n", arg2);
		else
		{
			connection store = all_slaves.get_a_connection();
			char temp[MAXLINE];
			snprintf(temp, MAXLINE, "upload %s %s %s %s %s", arg1, arg2, iter->second->get_clientname(), store.backup1.c_str(), store.backup2.c_str());
			int x = strlen(temp);
			if(ssl_writen(store.ssl, temp, x) != x) // ssl_writen error
			{
				log_msg("command_parse: ssl_writen error"); 
				snprintf( message, MAXLINE, "%s upload error\n", arg2);
			}
			else
			{
			    iter->second->add_file(arg2, arg3, store.address.c_str(), store.backup2.c_str(), store.backup2.c_str());
				snprintf(message, MAXLINE, "%s start to upload.\n", arg2);
			}
		}
			int len = strlen(message);
			if(ssl_writen(ssl, message, len) != len)
				log_msg("command_parse: SSL_writen error");
	}
	else if(strcmp(command, "download") == 0) // download a file 
	{
		//commandline format: upload ip_address(arg1) file_name(arg2)
		if(n != 3) // wrong command
			strcpy(message, "wrong command.\n");
		else if(!signed_in)// the user not signin.
			strcpy(message, "not signed in.\n");
		else
		{
			char storage[3][MAXLINE];
			int k = iter->second->query_file_storage(arg2, storage[0], storage[1], storage[2]);
			if(k == FILE_NOT_EXIST)
				strcpy(message, "file not exist.\n");
			else
			{
				// try to connect to the storage and backup storage.
				for(int i = 0; i < 3; ++i)
				{
					if(storage[0] == '\0')
						break;
					SSL *ssl_temp = all_slaves.is_exist(storage[i]);
					if(ssl_temp == NULL)
					{
						snprintf(message, MAXLINE, "storage server %s not connected.\n", storage[i]);
						continue;
					}
					else
					{
						char temp[MAXLINE];
						snprintf(temp, MAXLINE, "%s %s", command_line, iter->second->get_clientname());
						int x = strlen(temp);
						if(ssl_writen(ssl_temp, temp, x) != x)
						{
							log_msg(" command_parse: ssl_writen error");
							snprintf(message, MAXLINE, "%s download from %s error.\n", arg2, storage[i]);
						}
						else
						{
							snprintf(message, MAXLINE, "%s start to download from %s.\n", arg2, storage[i]);
							break;
						}
				 	}
				} 
			}
		}
		int len = strlen(message);
		if(ssl_writen(ssl, message, len) != len)
			log_msg("command_parse: ssl_writen error");
	}
	else if(strcmp(command, "delete") == 0) // delete a file
	{
		// commandline format: delete ip_address(arg1) file_name(arg2)
		if(n != 2) // wrong command 
			strcpy(message, "wrong command.\n");
		else if(!signed_in) // the user not signin.
			strcpy(message, "not signed in.\n");
		else
		{
			char storage[3][MAXLINE];
			int k = iter->second->query_file_storage(arg1, storage[0], storage[1], storage[2]);
			if(k == FILE_NOT_EXIST) // the file not exist in the database.
				strcpy(message, "file not exist.\n");
			else
			{
				bool success = false;
				for(int i = 0; i < 3; ++i)
				{
					if(storage[i] == '\0')
						break;
					SSL *ssl_temp = all_slaves.is_exist(storage[i]); 
					if(ssl_temp == NULL) // the very slav is not connected.
					{
						snprintf(message, MAXLINE, "storage server %s not connected.\n", storage[i]);
						continue;
					}
					else
					{
						char temp[MAXLINE];
						snprintf(temp, MAXLINE, "%s %s", command_line, iter->second->get_clientname());
						int x = strlen(temp);
						if(ssl_writen(ssl_temp, temp, x) != x) // send the slave server the message to delete a file
						{
							log_msg("command_parse: ssl_write error");
							snprintf(message, MAXLINE, "%s delete error.\n", arg1);
						}
						else
						{
							success = true;
						    log_msg("%s from %s deleted.\n", arg1, storage[i]);
						} 
					}
				}
				iter->second->delete_file(arg1);
		        if(success)
			       snprintf(message, MAXLINE, "%s deleted successfully\n", arg1);
		        else
			       snprintf(message, MAXLINE, "%s deleted unsuccessfully\n", arg2);
			}
		}
		int len = strlen(message);
		if(ssl_writen(ssl, message, len) != len)
			log_msg("command_parse: ssl_writen error");
	}
	else // unknown command
	{
		strcpy(message, "unknown command.\n");
		int len = strlen(message);
		if(ssl_writen(ssl, message, len) != len)
			log_msg("command_parse: ssl_writen error");
	}
}

