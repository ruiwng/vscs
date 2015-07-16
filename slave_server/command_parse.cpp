/*************************************************************************
	> File Name: command_parse.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 13 Jul 2015 09:10:33 PM CST
 ************************************************************************/
#include  "csd.h"
#include  "command_parse.h"

// delete a file
void delete_file(const char *command_line)
{
	char user_name[MAXBUF], file_name[MAXBUF];
	if(sscanf(command_line, "%s%s", user_name, file_name) != 2)
	{
		log_msg("delete_file: parse command unsuccessfully");
		return;
	}
	char file[MAXBUF];
	snprintf(file, MAXBUF, "%s/%s/%s", STORE_PATH, user_name, file_name);
	// delete the file.
	if(unlink(file) < 0)
	{
		log_ret("delete_file: unlink error");
		return;
	}
	log_msg("delete_file: delete file successfully");
}

//download the file
void *download_thread(void *command_line)
{
	char ip_addr[MAXBUF], user_name[MAXBUF], file_name[MAXBUF];
	if(sscanf((char *)command_line, ip_addr, user_name, file_name) != 3)
	{
		log_msg("download_thread: parse command unsuccessfully");
		return NULL;
	}
	char file[MAXBUF];
	snprintf(file, MAXBUF,"%s/%s/%s", STORE_PATH, user_name, file_name);
	FILE *p_file;
	if((p_file = fopen(file, "r")) == NULL)// open the file and check if it exists or not 
	{
		log_msg("download_thread: file %s doesn't exist", file);
		return NULL;
	}
	// initialize the connection.
	int sock_fd;
	if((sock_fd=socket(AF_INET,SOCK_STREAM, 0)) < 0)
	{
		fclose(p_file);
		log_ret("download_thread: socket error");
		return NULL;
	}
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(CLIENT_DOWNLOAD_PORT);
	if(inet_pton(AF_INET, ip_addr, &client_addr.sin_addr)< 0)
	{
		fclose(p_file);
		close(sock_fd);
		log_ret("download_thread: inet_pton error");
		return NULL;
	}
	// connect to the client.
	if(connect(sock_fd, (sockaddr*)&client_addr,sizeof(client_addr))<0)
	{
		fclose(p_file);
		close(sock_fd);
		log_ret("download_thread: connect error");
		return NULL;
	}
	//start to transmit the file
	char str_buf[MAXBUF];
	int nread;
	while((nread = fread(str_buf, sizeof(char), MAXBUF, p_file) > 0))
		writen(sock_fd, str_buf, nread);
	// close the socket and the open file.
	close(sock_fd);
	fclose(p_file);
	log_msg("download_thread: file %s tranmitted successfully", file);

	return NULL;
}

// upload the file
void *upload_thread(void *command_line)
{
	char ip_addr[MAXLINE], user_name[MAXLINE], file_name[MAXLINE];
	//fetch the ip address, user name and the file name
	if(sscanf((char*)command_line, "%s%s%s", ip_addr, user_name, file_name) != 3)
	{
		log_msg("upload_thread: parse command unsuccessfully");
		return NULL;
	}
	char file[MAXBUF];
	snprintf(file, MAXBUF, "%s/%s/", STORE_PATH, user_name);
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
	int sock_fd;
	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		log_ret("upload_thread: sock error");
		fclose(p_file);
		return NULL;
	}
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port =htons(CLIENT_UPLOAD_PORT);
	if(inet_pton(AF_INET, ip_addr, &client_addr.sin_addr)< 0)
	{
		log_ret("upload_thread: inet_pton error");
		fclose(p_file);
		close(sock_fd);
		return NULL;
	}
	//connect to the client
	if(connect(sock_fd, (sockaddr*)&client_addr, sizeof(client_addr)) < 0)
	{
		log_ret("upload_thread: connect to %s error", ip_addr);
		fclose(p_file);
		close(sock_fd);
	}
	//start to receive the file
	char str_buf[MAXBUF];
	int nread;
	while(true)
	{
		if((nread = read(sock_fd, str_buf, MAXBUF)) < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				log_ret("upload_thread: read error");
				fclose(p_file);
				close(sock_fd);
				return NULL; 
			}
		}
		if(nread == 0) // connection to the client was broken.
			break;
		fwrite(str_buf, sizeof(char), nread, p_file);
	}
	// close the socket and the open file.
	close(sock_fd);
	fclose(p_file);
	log_msg("upload_thread: upload successfully");

	return NULL;
}

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
	pthread_t thread;
	switch(command)
	{
		case REMOVE_FILE:
			delete_file(p);
			break;
		case DOWNLOAD_FILE:
			pthread_create(&thread, NULL, download_thread, (void *)p);
			break;
		case UPLOAD_FILE:
			pthread_create(&thread, NULL, upload_thread, (void *)p);
			break;
		default:
			{
				log_msg("command_parse: unknown command");
				return; 
			}
	}
}
