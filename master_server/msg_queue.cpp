/*************************************************************************
	> File Name: message_queue.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 17 Jul 2015 06:48:26 PM CST
 ************************************************************************/
#include  "msg_queue.h"

//constructor function
msg_queue::msg_queue()
{
	sem_init(&queue_sem, 0, 0);
    pthread_mutex_init(&queue_mutex, NULL);
}

//destructor function
msg_queue::~msg_queue()
{
	sem_destroy(&queue_sem);
	pthread_mutex_destroy(&queue_mutex);
}

void msg_queue::pop_msg(int &sock,SSL *&s, char *message)
{
	sem_wait(&queue_sem);//wait until the queue is not empty

	pthread_mutex_lock(&queue_mutex);// lock the mutex
	const msg &temp = all_messages.front();
	sock = temp.sockfd;
	s = temp.ssl;
	strcpy(message, temp.message.c_str());
	all_messages.pop();// get the message out of the queue
	pthread_mutex_unlock(&queue_mutex);// unlock the mutex
}

void msg_queue::push_msg(int sock, SSL *s, char *message)
{
	pthread_mutex_lock(&queue_mutex);// lock the mutex 

	all_messages.push(msg(sock, s, message));//add a message to the queue
	pthread_mutex_unlock(&queue_mutex);//unlock the mutex

	sem_post(&queue_sem);
}


