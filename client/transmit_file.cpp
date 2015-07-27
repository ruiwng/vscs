/*************************************************************************
	> File Name: transmit_file.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 20 Jul 2015 02:15:10 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "transmit_file.h"
#include  "client.h"
#include  <stack>
#include  <termios.h>

// exclusive access to the download array.
bool status_query;
extern pthread_mutex_t download_mutex;
extern unordered_map<string, record> download_array; // all download files
//exclusive access to the upload array.
extern pthread_mutex_t upload_mutex;
extern unordered_map<string, record> upload_array; // all upload files.

//get the progress of the upload/download files.
void get_progress(const char *operation, const char *file_name, long long prev, long long curr, long long sum, char *result)
{
	int pos = (double)curr/sum*50;
	snprintf(result, MAXLINE, "%-13s%-30s", operation, file_name);
	char *p = result + strlen(result);
	*p++ = '[';
	for(int i = 0; i < pos; ++i)
		*p++ = '=';
		*p++ = '>';
	for(int i = pos; i < 49; ++i)
		*p++ = ' ';
	*p++ = ']';

	stack<long long> num_stack;
	long long num = curr;
	do
	{
		num_stack.push(num%1000);
		num /= 1000;
	}while(num != 0);
	char temp[MAXLINE];
	char *q = temp;
	bool first = true;
	while(num_stack.size() != 1)
	{
		if(first)
		{
			first = false;
			sprintf(q, "%3lld,", num_stack.top());
		}
		else
		    sprintf(q, "%03lld,",num_stack.top());
		num_stack.pop();
		q += strlen(q);
	}
	if(first)
	{
		first = false;
		sprintf(q, "%3lld,", num_stack.top());
	}
	else
	    sprintf(q, "%03lld", num_stack.top());
	sprintf(p, "%15s", temp);
	p += strlen(p);
	snprintf(temp, MAXLINE, "%15.1lfK/s\n", (double)(curr - prev)/500);
	strcpy(p, temp); 
}

// query the status of the download/upload files.
void* status_thread(void *arg)
{
	//initialize the prev size.
	pthread_mutex_lock(&download_mutex);
	//update the prev to the current in the download array.
	for(unordered_map<string, record>::iterator iter = download_array.begin(); iter != download_array.end();
			++ iter)
	   iter->second.prev = iter->second.current;
	pthread_mutex_unlock(&download_mutex);
	
	//update the prev to the current in the upload array.
	pthread_mutex_lock(&upload_mutex);
	for(unordered_map<string, record>::iterator iter = upload_array.begin(); iter != upload_array.end();
			++ iter)
		iter->second.prev = iter->second.current;
	pthread_mutex_unlock(&upload_mutex);
	sleep_us(500000);
	bool empty = false;
	int move_up = 0;
	while(status_query)
	{
		for(int i = 0; i< move_up; ++i)
		{
			printf("\033[1A");
			printf("\033[K");
		}
		move_up = 0;
		string result;
		char temp[MAXLINE];
		pthread_mutex_lock(&download_mutex);
		if(download_array.empty())
			empty = true;
		// show the progress of downloading files.
		for(unordered_map<string, record>::iterator iter= download_array.begin(); iter != download_array.end();
				++ iter)
		{
			get_progress("downloading", iter->first.c_str(), iter->second.prev, iter->second.current, iter->second.sum, temp);
			iter->second.prev = iter->second.current;
			result += temp;
		    ++move_up;
		}
		pthread_mutex_unlock(&download_mutex);
		pthread_mutex_lock(&upload_mutex);
		if(empty && upload_array.empty())
			empty = true;
		else
			empty = false;
		// show the progress of uploading files.
		for(unordered_map<string, record>::iterator iter = upload_array.begin(); iter != upload_array.end();
				++ iter)
		{
			get_progress("uploading", iter->first.c_str(), iter->second.prev, iter->second.current, iter->second.sum, temp);
			iter->second.prev = iter->second.current;
			result += temp;
			++move_up;
		}
		pthread_mutex_unlock(&upload_mutex);
		if(empty)
			break;
		
		printf("%s", result.c_str());
		fflush(stdout);
		sleep_us(500000);
	}
	if(empty)
	{
		printf("no download/upload files");
		status_query = false;
	}
	return NULL;
}
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
	SSL_write(down_arg->ssl, "OK", strlen("OK"));
	char str_buf[MAXBUF];
	int nread;
	long long download_bytes = 0;
	int k = 0;
	while(true)
	{
		if((nread = SSL_read(down_arg->ssl, str_buf, MAXBUF)) < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				log_ret("download_thread: SSL_read error");
				fclose(p_file);
				SSL_shutdown(down_arg->ssl);
				close(down_arg->sockfd);
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
	// last time updating the size downloaded.
	pthread_mutex_lock(&download_mutex);
	down_arg->iter->second.current = download_bytes;
	pthread_mutex_unlock(&download_mutex);
	// close the socket descriptor, the SSL, and free the memory.
	SSL_shutdown(down_arg->ssl);
	close(down_arg->sockfd);
	SSL_free(down_arg->ssl);

	// whether download successfully, or not.
	bool success;
	pthread_mutex_lock(&download_mutex);
	success = (down_arg->iter->second.current == down_arg->iter->second.sum);
	download_array.erase(down_arg->iter);
	pthread_mutex_unlock(&download_mutex);
	if(success)
	{
		if(!status_query)
		   printf("%s download successfully.\n", down_arg->file_name);
	}
	else
	{
		if(!status_query)
		   printf("%s download unsuccessfully.\n", down_arg->file_name);
		unlink(down_arg->file_name);
	}
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
		SSL_shutdown(up_arg->ssl);
		close(up_arg->sockfd);
		SSL_free(up_arg->ssl);
		delete up_arg;
		printf("upload_thread: open %s for read unsuccessfully\n", up_arg->file_name);
		return NULL;
	}
	char str_buf[MAXBUF];
	int nread;
	long long upload_bytes = 0;
	snprintf(str_buf, MAXBUF, "%lld", up_arg->iter->second.sum);
	SSL_write(up_arg->ssl, str_buf, strlen(str_buf));
	if((nread = SSL_read(up_arg->ssl, str_buf, MAXBUF)) < 0)
	{
		SSL_shutdown(up_arg->ssl);
		close(up_arg->sockfd);
		SSL_free(up_arg->ssl);
		printf("upload_thread: SSL_read error\n");
		delete up_arg;
		return NULL;
	}
	str_buf[nread] = '\0';
	if(strcmp(str_buf, "OK") != 0)
	{
		SSL_shutdown(up_arg->ssl);
		close(up_arg->sockfd);
		SSL_free(up_arg->ssl);
		printf("upload_thread: strcmp error\n");
		delete up_arg;
		return NULL;
	}
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

	// last time updating the size uploaded.
	pthread_mutex_lock(&upload_mutex);
	up_arg->iter->second.current = upload_bytes;
	pthread_mutex_unlock(&upload_mutex);
	// close the socket descriptor, the SSL, and free the memory.
	SSL_shutdown(up_arg->ssl);
	close(up_arg->sockfd);
	SSL_free(up_arg->ssl);

	// whether upload successfully, or not.
	bool success;
	pthread_mutex_lock(&upload_mutex);
	success = (up_arg->iter->second.current == up_arg->iter->second.sum);
	upload_array.erase(up_arg->iter);
	pthread_mutex_unlock(&upload_mutex);

	if(success)
	{
		if(!status_query)
	 	    printf("%s upload successfully.\n", up_arg->file_name);
	}
	else
	{
		if(!status_query)
		    printf("%s upload unsuccessfully.\n", up_arg->file_name);
	}

	delete up_arg;

	return NULL;
}

