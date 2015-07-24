/*************************************************************************
	> File Name: sleepus.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Fri 24 Jul 2015 08:09:09 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  <sys/time.h>

void sleep_us(unsigned int nusecs)
{
	struct timeval tval;

	tval.tv_sec = nusecs / 1000000;
	tval.tv_usec = nusecs % 1000000;
	select(0, NULL, NULL, NULL, &tval);
}
