/*************************************************************************
	> File Name: file_verify.cpp
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Wed 05 Aug 2015 07:49:02 PM CST
 ************************************************************************/
#include  "vscs.h"

// translate unsigned char * into char *.
char *hexstr(unsigned char *buf)
{
	const char *set = "0123456789abcdef";
	char *str = (char *)malloc(sizeof(char) * 60);
	char *tmp = str;
	for(int i = 0; i < SHA_DIGEST_LENGTH; ++i)
	{
		*tmp++ = set[ (*buf) >> 4 ];
		*tmp++ = set[ (*buf) & 0xF ];
		buf ++;
	}
	*tmp = '\0';
	return str;
}

//if success, return the verification, otherwise return NULL.
//remember to release the memory the return pointer point to.
char *file_verify(const char *file_name)
{
	// open the file.
	FILE *p_file = fopen(file_name, "r");
	
	if(p_file == NULL)
	{
		log_msg("file_verify: open %s error", file_name);
		return NULL;
	}
	SHA_CTX ctx;
	if(!SHA1_Init(&ctx))
	{
		fclose(p_file);
		log_msg("file_verify: SHA1_Init error");
		return NULL;
	}

	unsigned char *temp = (unsigned char *)malloc((SHA_DIGEST_LENGTH + 1) * sizeof(unsigned char));
	memset(temp , 0, SHA_DIGEST_LENGTH + 1);
	char buf[MAXLINE + 1];
	while(true)
	{
		// read the file and add the verification.
		int nread = fread( buf, sizeof(char), MAXLINE, p_file);
		if(nread < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				log_msg("file_verify: read file error");
				free(temp);
				fclose(p_file);
				return NULL;
			}
		}
		else if(nread == 0)
			break;
		SHA1_Update(&ctx, buf, nread);
	}
	SHA1_Final(temp, &ctx);

	fclose(p_file);
	OPENSSL_cleanse(&ctx, sizeof(ctx));

	/*
	 * transfrom unsigned char* to char*
	*/
	char *result = hexstr(temp);
	free(temp);
	return result;
}
