/*************************************************************************
	> File Name: slave_config.h
	> Author: ruiwng
	> Mail: ruiwng@zju.edu.cn 
	> Created Time: Sat 18 Jul 2015 08:52:50 PM CST
 ************************************************************************/
#ifndef   SLAVE_CONFIG_H
#define   SLAVE_CONFIG_H

//read the slave configure file, include the port of the slave server, the status port of the slave 
//server, file back up port, the transmit port of the client, port wait for a connection to be a slave server.
//certificate file of the ssl, the private key of SSL,
// and the directory to store files.
//if success return zero, otherwise return -1.

int slave_configure(char *slave_port, char *slave_status_port, char *backup_port, char *client_transmit_port, 
		char *slave_listen_port, char *ssl_certificate, char *ssl_key,char *store_dir);

#endif  // SLAVE_CONFIG_H
