/*************************************************************************
	> File Name: client_config.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sun 19 Jul 2015 06:47:25 PM CST
 ************************************************************************/
#ifndef   CLIENT_CONFIG_H
#define   CLIENT_CONFIG_H

// read the configure file, configure the following arugments, the port of the master server.
// the file transmit port, ssl certificate file, and the sll private key.
int client_configure(char *master_port, char *transmit_port, char *ssl_certificate, char *ssl_key);

#endif
