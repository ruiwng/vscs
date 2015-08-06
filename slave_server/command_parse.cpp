/*************************************************************************
	> File Name: command_parse.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 09:10:33 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "command_parse.h"

extern SSL_CTX *ctx_client;
extern char client_transmit_port[MAXLINE];
extern char store_dir[MAXLINE];
extern char backup_port[MAXLINE];

// to realize the exclusive access to the download arary.
extern pthread_mutex_t download_mutex;
extern unordered_set<string> download_array;

// to realize the exclusive access to the upload array.
extern pthread_mutex_t upload_mutex;
extern unordered_set<string> upload_array;

// delete a file
void delete_file(char *command_line)
{
	log_msg("enter delete_file");
	log_msg("store_dir %s", store_dir);
	char user_name[MAXBUF], file_name[MAXBUF];
	if(sscanf(command_line, "%s%s", file_name, user_name) != 2)
	{
		log_msg("delete_file: parse command unsuccessfully");
		free(command_line);
		return;
	}
	free((void *)command_line);
	char file[MAXBUF + 1];
	snprintf(file, MAXBUF, "%s/%s/%s", store_dir, user_name, file_name);
	// delete the file.
	if(unlink(file) < 0)
	{
		log_ret("delete_file: unlink error");
		return;
	}
	log_msg("delete_file: delete file successfully");
}

//the client download file from the master server.
void *download_thread(void *command_line)
{
	char ip_addr[MAXBUF], file_name[MAXBUF], user_name[MAXBUF];
	if(sscanf((char *)command_line, "%s%s%s",ip_addr, file_name, user_name) != 3)
	{
		log_msg("download_thread: %s", command_line);
		free(command_line);
		log_msg("download_thread: parse command unsuccessfully");
		return NULL;
	}
	free(command_line);
	char file[MAXBUF + 1];
	snprintf(file, MAXBUF,"%s/%s/%s", store_dir, user_name, file_name);
	FILE *p_file;
	if((p_file = fopen(file, "r")) == NULL)// open the file and check if it exists or not 
	{
		log_msg("download_thread: file %s doesn't exist", file);
		return NULL;
	}
	// initialize the connection.
	int sock_fd = client_connect(ip_addr, client_transmit_port);

	if(sock_fd < 0)
	{
		fclose(p_file);
		log_msg("download_thread: can't connect to %s:%s", ip_addr, client_transmit_port);
		return NULL;
	}
	SSL *ssl = ssl_client(ctx_client, sock_fd);

	if(ssl == NULL)
	{
		fclose(p_file);
		close(sock_fd);
		log_msg("dowload_thread: ssl_server error");
		return NULL;
	}
	
	// to make sure this is the very file that the client want to download.
	char str_buf[MAXBUF];
	int nread;
	struct stat64 s;
	fstat64(fileno(p_file), &s);
	snprintf(str_buf,MAXBUF, "download %s %lld", file_name,(long long)s.st_size);
	if(SSL_write(ssl, str_buf, strlen(str_buf)) != (int)strlen(str_buf))
	{
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("download_thread: SSL_write error");
		return NULL;
	}
	if((nread = SSL_read(ssl, str_buf, MAXBUF)) < 0)
	{
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("download_thread: SSL_read error");
		return NULL;
	}

	str_buf[nread] = '\0';
	if(strcmp(str_buf,"OK") != 0)
	{
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("download_thread: strcmp unsuccessfully");
		return NULL;
	}
	// insert the download job to the download array.
	pthread_mutex_lock(&download_mutex);
	download_array.insert((const char *)command_line);
	pthread_mutex_unlock(&download_mutex);

	//start to transmit the file
	while((nread = fread(str_buf, sizeof(char), MAXBUF, p_file)) > 0)
	{
		if(SSL_write(ssl, str_buf, nread) != nread)
		{
			SSL_shutdown(ssl);
			close(sock_fd);
			fclose(p_file);

			SSL_free(ssl);
			log_msg("download_thread SSL_write error");
			return NULL;
		}
	}
	// close the socket and the open file.
	
	// erase the download job from the download array.
	pthread_mutex_lock(&download_mutex);
	download_array.erase((const char *)command_line);
	pthread_mutex_unlock(&download_mutex);

	SSL_shutdown(ssl); // send SSL/TLS close_notify */
	close(sock_fd);
	fclose(p_file);
	
	SSL_free(ssl);
	log_msg("download_thread: file %s transmitted successfully", file);
	
	return NULL;
}

// the client upload file to the slave server.
void *upload_thread(void *command_line)
{
	 log_msg("upload_thread: %s start", command_line);
	char ip_addr[MAXLINE], file_name[MAXLINE], user_name[MAXLINE];
	char backup_addr[2][MAXLINE];
	//get the ip address user name, and file name of the client.
	int n = sscanf((char *)command_line, "%s%s%s%s%s", ip_addr, file_name, user_name, backup_addr[0], backup_addr[1]);
	if(n < 3)
	{
		free(command_line);
		log_msg("upload_thread: parse command unsuccessfully");
		return NULL;
	}
	else if(n == 3)
	{
		*backup_addr[0] = '\0';
		*backup_addr[1] = '\0';
		log_msg("upload_thread: no backup address");
	}
	else if(n == 4)
	{
		*backup_addr[1] = '\0';
		log_msg("upload_thread: one backup address");
	}
	else 
		log_msg("upload_thread: backup address %s", backup_addr);

	free(command_line);
	log_msg("upload_thread: %s", store_dir);
	char file[MAXBUF];

	// establish the directory to store files of the client.
	snprintf(file, MAXBUF, "%s/%s/", store_dir, user_name);
	log_msg("upload_thread: file directory %s", file);
	if(mkdir(file, DIR_MODE) < 0)
	{
		if(errno != EEXIST)
		{
			log_ret("upload_thread: mkdir error");
			return NULL;
		}
	}
	strcat(file, file_name);
	log_msg("upload_thread: file name %s", file); 

	// open the uploaded file to write.
	FILE *p_file;
	if((p_file = fopen(file, "w")) == NULL)
	{
		log_msg("upload_thread: open %s for write unsuccessfully", file);
		return NULL;
	} 

	//initialize the connection to the client.
	int sock_fd = client_connect(ip_addr, client_transmit_port);
	
	//fail in establishing a connection to the client.
	if(sock_fd < 0)
	{
		unlink(file);
		fclose(p_file);
		log_msg("upload_thread: cannot connect to %s:%s", ip_addr, client_transmit_port);
		return NULL;
	}
	//start to receive the file
	SSL *ssl;
	ssl = ssl_client(ctx_client, sock_fd);

	if(ssl == NULL)
	{
		unlink(file);
		fclose(p_file);
		close(sock_fd);
		log_msg("upload_thread: ssl_client error");
		return NULL;
	}
	char str_buf[MAXBUF];
	int nread;
	snprintf(str_buf, MAXBUF, "upload %s", file_name);
	log_msg("upload_thread: %s", str_buf);
	int len = strlen(str_buf);
	/*
	 * tell the client which file to upload
	 */
	if(SSL_write(ssl, str_buf, len) != len)
	{
		unlink(file);
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("upload_thread: SSL_write error");
		return NULL;
	}
	log_msg("upload_thread: %s", str_buf);
	if((nread = SSL_read(ssl, str_buf, MAXBUF)) < 0)
	{
		unlink(file);
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("upload_thread: SSL_read error");
		return NULL;
	}
	str_buf[nread] = '\0';
	log_msg("upload_thread: %s",str_buf);
	if(strcmp(str_buf,"ERR") == 0)
	{
		unlink(file);
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("upload_thread: strcmp error");
		return NULL;
	}
	long long upload_bytes;
	/*
	 * get the upload file's size.
	 */
	sscanf(str_buf, "%lld",&upload_bytes);
	SSL_write(ssl, "OK", strlen("OK"));

	/*
	 * connect to the backup slave server.
	 */
	int backup_fd[2];
	backup_fd[0] = -1;
	backup_fd[1] = -1;
	for(int i = 0; i < 2; ++i)
	{
		if(*backup_addr[i] !='\0')
	        backup_fd[i] = client_connect(backup_addr[i], backup_port);
	    else
	   	   break;
	    if(backup_fd[i] >= 0)
	    {
		    char backup_msg[MAXLINE + 1];
		    snprintf(backup_msg, MAXLINE, "%s %s %lld", file_name, user_name, upload_bytes);
		    int len = strlen(backup_msg);
		    int written = writen(backup_fd[i], backup_msg, len);
		    if(written != len)
		    {
			     backup_fd[i] = -1;
			     log_ret("upload_thread: writen to backup server error");
		    }
		    else
		    {
			     len = read(backup_fd[i], backup_msg, MAXLINE);
			     if(len < 0)
			     {
				     backup_fd[i] = -1;
	 			     log_ret("upload_thread: read from backup server error");
			     }
		      	else
			     {
	 			      backup_msg[len] = '\0';
				      if(strcmp(backup_msg, "OK") != 0)
				      {
	 				       backup_fd[i] = -1;
					       log_msg("upload_thread: backup strcmp error");
				      }
			     }
		    }
	   }
	   else
	   {
		    backup_fd[i] = -1;
		    log_msg("upload_thread: cannot connect to the backup server %s", backup_addr);
	   }
	}
	if(backup_fd[0] != -1)
		log_msg("upload_thread: connect to the backup server %s successfully", backup_addr[0]);
	else if(backup_addr[0][0] != '\0')
		log_msg("upload_thread: connect to the backup server %s unsuccessfully", backup_addr[0]);
	if(backup_fd[1] != -1)
		log_msg("upload_thread: connect to the backup server %s successfully", backup_addr[1]);
	else if(backup_addr[1][0] != '\0')
		log_msg("upload_thread: connect to the backup server %s unsuccessfully", backup_addr[1]);
	// add the upload job to the upload array.
	pthread_mutex_lock(&upload_mutex);
	upload_array.insert((const char *)command_line);
	pthread_mutex_unlock(&upload_mutex);

	while(true)
	{
		if((nread = SSL_read(ssl, str_buf, MAXBUF)) < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
	 		 	log_ret(" upload_thread: SSL_read error");
				unlink(file);
				fclose(p_file);
				SSL_shutdown(ssl);
				close(sock_fd);
				SSL_free(ssl);
	 			return  NULL; 
			}
		}
		if(nread == 0) // connection to the client was broken.
			break;
		fwrite(str_buf, sizeof(char), nread, p_file);

		// if backup server is connected, write to it.
		for(int i = 0; i < 2; ++i)
		{
			if(backup_fd[i] != -1)
		  {
			  int n = writen(backup_fd[i], str_buf, nread);
			  if(n != nread)
				   log_msg("upload_thread: write to the backup server error");
		  }
		}
		upload_bytes -= nread;
	}
	if(upload_bytes != 0)
	{
		unlink(file);
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("upload_thread: file size not equal: upload_bytes %lld", upload_bytes);
		return NULL;
	}
	//erase the upload job from the upload array.
	pthread_mutex_lock(&upload_mutex);
	upload_array.erase((const char *)command_line);
	pthread_mutex_unlock(&upload_mutex);

	SSL_shutdown(ssl); /* send SSL/TLS close_notify */
	// close the socket and the open file.
	close(sock_fd);
	fclose(p_file);
	SSL_free(ssl);
	log_msg("upload_thread: upload successfully");

	return NULL;
}
//parse the command received from the master server.
void command_parse(const char *command_line)
{
	log_msg("command_parse: %s", command_line);
	char command[MAXLINE];
	if(sscanf(command_line, "%s", command) != 1)
	{
		log_msg("command_parse: parse command unsuccessfully");
		return;
	}
	const char *p = command_line;
	while(*p == ' ')
		++p;
	p = strchr(p, ' ');
	if(p == NULL)
	{
		log_msg("command_parse: wrong command");
		return;
	}
	char *q = (char *)malloc(strlen(p) + 1);
	strcpy(q, p);
	pthread_t thread;
	log_msg("q information: %s, command %s", q, command);
	if(strcmp(command,"delete") == 0) //remove a file
	{
		log_msg("delete route");
		delete_file(q);
	}
	else if(strcmp(command, "download") == 0) // download a file from the slave server
	{
		log_msg("download route");
		pthread_create(&thread, NULL, download_thread, (void *)q);
	}
	else if(strcmp(command, "upload") == 0) // upload a file to the slave server.
	{
		log_msg("upload route");
		pthread_create(&thread, NULL, upload_thread, (void *)q);
	}
	else // unknown command
	{
		log_msg("unknown route");
		free(q);
		log_msg("command_parse: unknown command");
		return;
	}
}
