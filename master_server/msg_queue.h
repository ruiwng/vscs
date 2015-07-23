/*************************************************************************
	> File Name: msg_queue.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 17 Jul 2015 06:48:56 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  <queue>// inclusion for queue
#include  <semaphore.h>// inclusion for semaphore
#include  <string>

using namespace std;

class msg_queue
{
private:
	struct msg
	{
		SSL *ssl;
		string message;
		msg(SSL *s, char *m):ssl(s), message(m){}
	};
	queue<msg> all_messages;

public:
	msg_queue();
	~msg_queue();
	// get a message from the queue
	void pop_msg(SSL *&s, char *message);
	// push a message to the queue
	void push_msg(SSL *s, char *message);

private:
	sem_t queue_sem; // tell when the queue is not empty
	pthread_mutex_t queue_mutex;// pop and push the queue exclusively.
};

