/*************************************************************************
	> File Name: command_parse.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 09:10:33 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "command_parse.h"

extern SSL_CTX *ctx;
extern char *client_transmit_port;
extern char *store_dir;
// to realize the exclusive access to the download arary.
extern pthread_mutex_t download_mutex;
extern unordered_set<string> download_array;

// to realize the exclusive access to the upload array.
extern pthread_mutex_t upload_mutex;
extern unordered_set<string> upload_array;

// delete a file
static void delete_file(char *command_line)
{
	char user_name[MAXBUF + 1], file_name[MAXBUF + 1];
	if(sscanf(command_line, "%s%s", user_name, file_name) != 2)
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
static void *download_thread(void *command_line)
{
	char ip_addr[MAXBUF], user_name[MAXBUF], file_name[MAXBUF];
	if(sscanf((char *)command_line, ip_addr, user_name, file_name) != 3)
	{
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
	
	SSL *ssl = ssl_client(ctx, sock_fd);

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
	struct stat s;
	fstat(fileno(p_file));
	snprintf(str_buf,"download %s %d", file_name, s.st_size);
	if(SSL_write(ssl, str_buf, strlen(str_buf)) != strlen(str_buf))
	{
		fcloe(p_file);
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
	while((nread = fread(str_buf, sizeof(char), MAXBUF, p_file) > 0))
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
static void *upload_thread(void *command_line)
{
	char ip_addr[MAXLINE], user_name[MAXLINE], file_name[MAXLINE];
	//get the ip address user name, and file name of the client.
	if(sscanf((char*)command_line, "%s%s%s", ip_addr, user_name, file_name) != 3)
	{
		free(command_line);
		log_msg("upload_thread: parse command unsuccessfully");
		return NULL;
	}
	free(command_line);
	char file[MAXBUF];
	// establish the directory to store files of the client.
	snprintf(file, MAXBUF, "%s/%s/", store_dir, user_name);
	if(mkdir(file, DIR_MODE) < 0)
	{
		if(errno != EEXIST)
		{
			log_ret("upload_thread: mkdir error");
			return NULL;
		}
	}
	strcat(file, file_name);
	// open the uploaded file to write.
	FILE *p_file;
	if((p_file = fopen(file, "w")) == NULL)
	{
		log_msg("upload_thread: open %s for write unsuccessfully", file);
		return NULL;
	}
	//initialize the connection
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
	ssl = ssl_client(ctx, sock_fd);

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
	if(SSL_write(ssl, str_buf, strlen(str_buf) != strlen(str_buf)))
	{
		unlink(file);
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("upload_thread: SSL_write error");
		return NULL;
	}
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
	int upload_bytes;
	int m = sscanf(str_buf, "%d",&upload_bytes);
	SSL_write(ssl, "OK", strlen("OK"));
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
				log_ret("upload_thread: SSL_read error");
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
		upload_bytes -= nread;
	}
	if(upload_bytes != 0)
	{
		unlink(file);
		fclose(p_file);
		SSL_shutdown(ssl);
		close(sock_fd);
		SSL_free(ssl);
		log_msg("upload_thread: file size not equal");
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
	int command;
	if(sscanf(command_line, "%d", &command) != 1)
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
	switch(command)
	{
		case REMOVE_FILE: //remove a file
			delete_file(q);
			break;
		case DOWNLOAD_FILE://download a file from the slave server
			pthread_create(&thread, NULL, download_thread, (void *)q);
			break;
		case UPLOAD_FILE://upload a file to the slave server
			pthread_create(&thread, NULL, upload_thread, (void *)q);
			break;
		default:// unknown command
			{
				free(q); 
				log_msg("command_parse: unknown command");
				return;   
			}
	}
}
