/*************************************************************************
	> File Name: transmit_file.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 20 Jul 2015 02:15:22 PM CST
 ************************************************************************/
#ifndef   TRANSMIT_FILE_H
#define   TRANSMIT_FILE_H
// query the status of the download/upload files.
void* status_thread(void *arg);

// the thread to download a file.
void *download_thread(void *arg);

// the thread to upload a file.
void *upload_thread(void *arg);

#endif
