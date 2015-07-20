/*************************************************************************
	> File Name: client_info.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 11 Jul 2015 10:13:45 PM CST
 ************************************************************************/
#include  "vscs.h"
#include  "client_info.h"


client_info::client_info(int s, const char *name):sock_db(s),client_name(name)
{
	get_filelist();
}

client_info::~client_info()
{
	restore_filelist();
}

//add a file correpsonding to the current user.
int client_info::add_file(const char *name, int file_size,const char *storage)
{
	unordered_map<string,info>::iterator iter = file_storage.find(name);
	if(iter != file_storage.end())
		return FILE_ALREADY_EXIST;
	file_storage.insert(make_pair(name, info(storage, file_size)));
	return OK;
}

// delete a file corresponding to the current user.
int client_info::delete_file(const char *file_name)
{
	unordered_map<string,info>::iterator iter= file_storage.find(file_name);
	if(iter == file_storage.end())
		return FILE_NOT_EXIST;
	file_storage.erase(iter);
	return OK;
}

//get the file list of the current user.
void client_info::get_filelist()
{
	char query[MAXLINE];
	char reply[MAXLINE];
	snprintf(query, MAXLINE, "get %s%s\n", USER_PREFIX,client_name.c_str());
	int len = strlen(query);
	if(writen(sock_db, query, len) != len||read_s(sock_db, reply, MAXLINE) < 0)
	{		
		log_msg("%s get_filelist unsuccessfully",client_name.c_str());
		return;
	}
	int str_length;
	sscanf(reply,"$%d\n",&str_length);
	if(str_length == -1)// the file list doesn't exist.
		return;
	char *p_mem = (char *)malloc(str_length + 1);
	strcpy(p_mem, strchr(reply, '\n')+1);
	int n = strlen(p_mem);
	if(n < str_length)
	{
		if(readn(sock_db, p_mem + n, str_length-n) < 0)
  	    {
		    log_msg("%s get_filelist unsuccessfully",client_name.c_str());
		    return;
	    }
		char temp;
		while(read_s(sock_db, &temp, 1) == 1)
		{
			if(temp == '\n')
				break;
		}
	}
	p_mem[str_length] = '\0';

	char file_name[MAXLINE], storage[MAXLINE];
	int file_size;
	while(sscanf(p_mem, "%s %s %d", file_name, storage, &file_size) == 3)
	{
		file_storage.insert(make_pair(file_name,info(storage,file_size)));
		p_mem = strchr(p_mem,'\n');
		if(p_mem==NULL)
			break;
		++p_mem;
	}
	log_msg("%s get_filelist successfully", client_name.c_str());
}

//restore the file list of the current user.
int client_info::restore_filelist() const
{
	string result;
	char temp[MAXLINE];
	for(unordered_map<string,info>::const_iterator iter=file_storage.begin();
			iter != file_storage.end(); ++iter)
	{
		snprintf(temp, MAXLINE, "%d", iter->second.file_size);
		result+=iter->first+" "+iter->second.storage+" "+temp+"\\n";
	}
	string query = string("set ")+USER_PREFIX+client_name+" \""+result+"\"\n";
	char reply[MAXLINE];
	int len = query.length();
	if(writen(sock_db, query.c_str(), len) != len ||
				read(sock_db, reply, MAXLINE) < 0)
	{
		log_msg("%s restore_filelist unsuccessfully", client_name.c_str());
		return ERR_RESTORE_FILE;
	}
	if(strncmp(reply,"+OK",3)!=0)
	{
		log_msg("%s restore_filelist unsuccessfully",client_name.c_str());
		return ERR_RESTORE_FILE;
	}
	log_msg("%s resotre_filelist successfully",client_name.c_str());
	return OK;
}

// get the storage of the current file.
int client_info::query_file_storage(const char *file_name, char *storage) const
{
	unordered_map<string,info>::const_iterator iter = file_storage.find(file_name);
	if(iter == file_storage.end())
		return FILE_NOT_EXIST;
	strcpy(storage, iter->second.storage.c_str());
	return OK;
}

//show the file list of the current user.
//remember to free the stoarge after useage.
char *client_info::show_filelist() const
{
	string result;
	for(unordered_map<string,info>::const_iterator iter=file_storage.begin();
			iter!=file_storage.end();++iter)
	{
		char size[MAXLINE];
		snprintf(size, MAXLINE,"%dMB", iter->second.file_size);
		result+=iter->first+" "+size+"\n";
	}
	char *t = (char*)malloc(result.length()+1);
	strcpy(t, result.c_str());
	return t;
}


//add a user, if the user already exists, return ERR_USER_EXIST,
//if add user successfully, return OK,
//otherwise return ERR_USER_ADD.
int user_add(int sockdb, const char *name, const char *passwd)
{
	if(user_exist(sockdb, name) == OK)
	{
		log_msg("user %s already exists", name);
		return USER_ALREADY_EXIST;
	}
	char query[MAXLINE];
	char reply[MAXLINE];
	snprintf(query, MAXLINE, "set %s %s\n",name, passwd);
	int len = strlen(query);
	if(writen(sockdb, query, len )!= len
			|| read(sockdb, reply, MAXLINE) < 0)
	{
		log_msg("add user %s unsuccessfully", name);
		return ERR_USER_ADD;
	}

	if(strncmp(reply, "+OK",3)==0)
	{
		log_msg("add user %s successfully", name);		
		return OK;
	}
	else
	{
		log_msg("add user %s unsuccessfully", name);
		return ERR_USER_ADD;
	}
}

// judge whether the user already exists,
// if this is the case, return OK,
// otherwise return ERR.
int user_exist(int sockdb, const char *name)
{
	char query[MAXLINE];
	char reply[MAXLINE];
	snprintf(query, MAXLINE, "get %s\n", name);
	int len = strlen(query);
	if(writen(sockdb, query, len) != len
			||read(sockdb, reply, MAXLINE) < 0)
	{
		log_msg("user_exist error");
		return ERR;
	}
	if(strncmp(reply, "$-1",3)==0)
		return ERR;
	else
		return OK;
}

// user login
int user_login(int sockdb, const char *name, const char *passwd)
{
	char query[MAXLINE];
	char reply[MAXLINE];
	snprintf(query, MAXLINE, "get %s\n", name);
	int len = strlen(query);
	if(writen(sockdb, query, len) != len
			|| read(sockdb, reply, MAXLINE) < 0)
	{
		log_msg("user_login unsuccessfully");
		return ERR;
	}
	if(strncmp(reply, "$-1",3)==0)
		return USER_NOT_EXIST;
	int k;
	sscanf(reply,"$%d", &k);
	char *p=strchr(reply,'\n')+1;
	if(strncmp(p,passwd,k)==0)
		return OK;
	else
		return WRONG_PASSWD;
}
