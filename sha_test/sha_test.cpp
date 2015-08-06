/*************************************************************************
	> File Name: sha_test.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 05 Aug 2015 08:25:43 PM CST
 ************************************************************************/
#include  "vscs.h"

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("USAGE : %s <filename> \n", argv[0]);
		return -1;
	}
	char *result = file_verify(argv[1]);
	printf("%s\n", result);
	free(result);
	return 0;
}
