/*************************************************************************
	> File Name: file_backup.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 29 Jul 2015 02:47:12 PM CST
 ************************************************************************/
#include  "file_backup.h"
#include  "vscs.h"

extern char store_dir[MAXLINE];

// to realize the exclusive access to the backup array.
extern pthread_mutex_t backup_mutex;
extern unordered_set<string> backup_array;

void *backup_thread(void *arg)
{
	// get the socket descriptor and free the allocated memory.
	int backupfd = *(int *)arg;
	free(arg);

	char command_line[MAXLINE + 1];

	int n = read(backupfd, command_line, MAXLINE);
	if(n < 0)
	{
		close(backupfd);
		log_msg("backup_thread: read error");
		return NULL;
	}
	command_line[n] = '\0';

	// parse the command line
	char file_name[MAXLINE], user_name[MAXLINE];
	long long upload_bytes;
	n = sscanf(command_line, "%s%s%lld", file_name, user_name, &upload_bytes);

	if(n != 2)
	{
		close(backupfd);
		log_msg("backup_thread: parse the command line error");
		return NULL;
	}

	char file[MAXBUF];

	// establish the directory to store files of the client.
	snprintf(file, MAXBUF, "%s/%s/", store_dir, user_name);
	
	log_msg("backup_thread: file directory %s", file);

	if(mkdir(file, DIR_MODE) < 0)
	{
		if(errno != EEXIST)
		{
			close(backupfd);
			log_ret("backup_thread: mkdir error");
			return NULL;
		}
	}
	strcat(file, file_name);
	log_msg("upload_thread: file name %s", file);
	
	// open the backup file to write.
	FILE *p_file;
	if((p_file = fopen(file, "w")) == NULL)
	{
		close(backupfd);
		log_msg("upload_thread: open %s for write unsuccessfully", file);
		return NULL;
	}

	n = writen(backupfd, "OK", strlen("OK"));
	
	if(n != strlen("OK"))
	{
		close(backupfd);
		fclose(p_file);
		return NULL;
	}

	int nread;
	// in the process of backing up file
	
	// add the backup job to the backup array.
	pthread_mutex_lock(&backup_mutex);
	backup_array.insert((const char *)command_line);
	pthread_mutex_unlock(&backup_mutex);

	char str_buf[MAXBUF + 1];
	// start backing up.
	while(true)
	{
		if((nread = read(backupfd, str_buf, MAXBUF)) < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				log_ret("backup_thread: read error");
				unlink(file);
				fclose(p_file);
				close(backupfd);
			}
		}
		else if(nread == 0) //connection to the client was broken.
			break;
		fwrite(str_buf, sizeof(char), nread, p_file);
		upload_bytes -= nread;
	}

	if(upload_bytes != 0)
	{
		unlink(file);
		fclose(p_file);
		close(backupfd);
		log_msg("backup_thread: file size not equal: upload_bytes %lld", upload_bytes);
		return NULL;
	}

	//erase the backup job from the backup array.
	pthread_mutex_lock(&backup_mutex);
	backup_array.erase((const char *)command_line);
	pthread_mutex_unlock(&backup_mutex);

	close(backupfd);
	fclose(p_file);
	
	log_msg("backup_thread: backup successfully");

	return NULL;
}
