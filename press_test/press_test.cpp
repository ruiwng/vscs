/*************************************************************************
	> File Name: press_test.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Mon 17 Aug 2015 04:36:16 PM CST
 ************************************************************************/
#include  <stdio.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <signal.h>

#define  LIMIT    10000

int i;

pid_t *pid_arry = (pid_t*)malloc(sizeof(int) * LIMIT);
int main()
{
	for(i = 0; i < LIMIT; ++i)
	{
		int m;
		if((m = fork()) < 0)
		{
			printf("fork error\n");
			break;
		}
		else if(m == 0)
		{
			execl("./vscs_client", "vscs_client", "10.66.27.245", (char *)0);
			exit(0);
		}
		else
			pid_arry[i] = m;
		sleep(1);
	}
	getchar();

	for(int j = 0; j < i; ++j)
		kill(pid_arry[j], 9);
	return 0;
}
