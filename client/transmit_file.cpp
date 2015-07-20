/*************************************************************************
	> File Name: transmit_file.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 20 Jul 2015 02:15:10 PM CST
 ************************************************************************/
#include  "transmit_file.h"
#include  "client.h"

// exclusive access to the download array.
extern pthread_mutex_t download_mutex;
extern unordered_map<string, record> download_array; // all download files
//exclusive access to the upload array.
extern pthread_mutex_t upload_mutex;
extern unordered_map<string, record> upload_array; // all upload files.

// download file from the slave server.
void *download_thread(void *arg)
{
	transmit_arg *down_arg = (transmit_arg*)arg;
	// open the download file to write.
	FILE *p_file;
	if((p_file = fopen(down_arg->file_name, "w")) == NULL)
	{
		close(down_arg->sockfd);
		SSL_free(down_arg->ssl);
		delete down_arg;
		printf("download_thread: open %s for write unsuccessfully", down_arg->file_name);
		return NULL;
	}

	char str_buf[MAXBUF];
	int nread, download_bytes = 0;
	int k = 0;
	while(true)
	{
		if((nread = SSL_read(down_arg->ssl, str_buf, MAXBUF)) < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				log_ret("upload_thread: SSL_read error");
				fclose(p_file);
				SSL_free(down_arg->ssl);
				delete down_arg;
			}
		}
		if(nread == 0) //connection to the client was broken.
		   break;
		fwrite(str_buf, sizeof(char), nread, p_file);
		download_bytes += nread;
		// every ten times writing to the file, record the bytes already downloaded in the download array.
		if( ++k == 20)
		{
			k = 0;
			pthread_mutex_lock(&download_mutex);
			down_arg->iter->second.current = download_bytes;
			pthread_mutex_unlock(&download_mutex);
		}
	}
	// close the socket descriptor, the SSL, and free the memory.
	close(down_arg->sockfd);
	SSL_free(down_arg->ssl);

	// whether download successfully, or not.
	pthread_mutex_lock(&download_mutex);
	if(down_arg->iter->second.current != down_arg->iter->second.sum)
		printf("%s download unsuccessfully.\n", down_arg->file_name);
	else
		printf("%s download successfully.\n", down_arg->file_name);
	download_array.erase(down_arg->iter);
	pthread_mutex_unlock(&download_mutex);

	delete down_arg;
	
	return NULL;
}

// upload file to the slave server.
void *upload_thread(void *arg)
{
	transmit_arg *up_arg = (transmit_arg*)arg;

    // open the upload file to read.
	FILE *p_file;
	if((p_file = fopen(up_arg->file_name, "r")) == NULL)
	{
		close(up_arg->sockfd);
		SSL_free(up_arg->ssl);
		delete up_arg;
		printf("upload_thread: open %s for read unsuccessfully", up_arg->file_name);
	}

	char str_buf[MAXBUF];
	int nread, upload_bytes = 0;
	int k =0;
	while((nread = fread(str_buf, sizeof(char), MAXBUF, p_file)) > 0)
	{
		if(SSL_write(up_arg->ssl, str_buf, nread) < 0)
				break;
		upload_bytes += nread;
		// every ten times reading the file, record the bytes alreadys uploaded in the upload array.
		if( ++k == 20)
		{
			k = 0;
			pthread_mutex_lock(&upload_mutex);
			up_arg->iter->second.current = upload_bytes;
			pthread_mutex_unlock(&upload_mutex);
		}
	}
	// close the socket descriptor, the SSL, and free the memory.
	close(up_arg->sockfd);
	SSL_free(up_arg->ssl);

	// whether upload successfully, or not.
	pthread_mutex_lock(&upload_mutex);
	if(up_arg->iter->second.current != up_arg->iter->second.sum)
		printf("%s upload unsuccessfully.\n", up_arg->file_name);
	else
		printf("%s upload successfully.\n", up_arg->file_name);
	download_array.erase(up_arg->iter);
	pthread_mutex_unlock(&download_mutex);

	delete up_arg;

	return NULL;
}

