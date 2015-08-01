/*************************************************************************
	> File Name: client_info.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 11 Jul 2015 10:13:45 PM CST
 ************************************************************************/
#ifndef   CLIENT_INFO_H
#define   CLIENT_INFO_H
#include  <unordered_map>
#include  <string>
using namespace std;

// the file information is represented as "file_name file_size storage1 storage2"
// e.g. abgbebt 10535 10.66.27.242 10.66.27.245
class client_info
{
private:
	
struct info
{
	long long file_size;
	string storage1;
	string storage2;
	info(long long size, const string& stor1, const string& stor2):file_size(size), storage1(stor1), storage2(stor2){}
};
public:
	client_info(int sockdb,const char *name);
	~client_info();

public:
	//add a file correspending to the current user.
	int add_file(const char *file_name, long long file_size, const char *storage1, const char *storage2);
	//delete a file corresponding to the current user.
	int delete_file(const char *file_name);
	//show the file list of the current user.
	//remember to free the memory after its usage.
	char *show_filelist() const;
	// Judge whether a file exists or not.
	bool is_file_exist(const char *name) const
	{
		return file_storage.find(name) != file_storage.end();
	}
	int query_file_storage(const char *name, char *storage1, char *storage2) const;
	const char *get_clientname() const {return client_name.c_str();}

private:
    void get_filelist();
	int restore_filelist() const;
private:
	unordered_map<string,info> file_storage; // file name vs its info
	long long total_file_size; // the size of all the files of the current user.
	const int sock_db; /* the socket connect to the db */
	const string client_name; /* the name of the user */
};

// add a user, if the user already exists, return ERR_USER_EXIST,
// if add user successfully, return 0,
// otherwise return ER_USER_ADD.
int user_add(int sockdb, const char *name, const char *passwd);

// judge the user exist or not.
// if this is the case, return OK,
// otherwise return -1.
int user_exist(int sockdb, const char *name);

// user login
// the return value USER_NOT_EXIST indicates the user doesn't exist.
// the return value WRONG_PASSWD indicates the password is wrong.
int user_login(int sockdb, const char *name, const char *passwd);

#endif // CLIENT_INFO_H
