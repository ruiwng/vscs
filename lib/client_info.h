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
// the file information is represented as "file_name storage file_size"
// e.g. abgbebt 10.66.27.242 10535
class client_info
{
private:
	
struct info
{
	string storage;
	int file_size;
	info(const string& stor, int size):storage(stor),file_size(size){}
};
public:
	client_info(int sockdb,const char *name);
	~client_info();

public:
	//add a file correspending to the current user.
	int add_file(const char *file_name, int file_size, const char *storage);
	//delete a file corresponding to the current user.
	int delete_file(const char *file_name);
	//show the file list of the current user.
	//remember to free the memory after its usage.
	char *show_filelist() const;
	int query_file_storage(const char *name, char *storage) const;
private:
    void get_filelist();
	int restore_filelist() const;
private:
	unordered_map<string,info> file_storage; // file name vs its info
	int total_file_size; // the size of all the files of the current user.
	const int sock_db; /* the socket connect to the db */
	const string client_name; /* the name of the user */
};

// add a user
int user_add(int sockdb, const char *name, const char *passwd);

// judge the user exist or not.
int user_exist(int sockdb, const char *name);

// user login
int user_login(int sockdb, const char *name, const char *passwd);
#endif // CLIENT_INFO_H
