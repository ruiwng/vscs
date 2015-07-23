#include  "vscs.h"
#include  <iostream>

#define SERVER_PORT    5003


#define SERVER_CERTIFICATE   "cacert.pem"    
#define SERVER_KEY           "privatekey.pem"

 
int main( int argc, char* argv[] ) {
  int ret;

  SSL_CTX* ctx = ssl_server_init(SERVER_CERTIFICATE, SERVER_KEY);
  int listen_socket;
  int accept_socket;

  listen_socket = server_listen("5003");  
  if( listen_socket < 0 ) {
    std::cout<<"socket error."<<std::endl;
    return -1;
  }
  sockaddr_in addr_client;

  socklen_t addr_client_len = sizeof(addr_client);
    accept_socket = accept (listen_socket, (struct sockaddr*) &addr_client, &addr_client_len);
  if( accept_socket < 0  ) {
    std::cout<<"accept error."<<std::endl;
    return -1;
  }
  close(listen_socket);

  SSL *ssl = ssl_server(ctx, accept_socket);
  char     buf [4096];

  ret = SSL_read (ssl, buf, sizeof(buf) - 1);    
  if( ret == -1 ) {
    std::cout<<"SSL_read error."<<std::endl;
    return -1;
  }
  buf[ret] = '\0';
  std::cout<<buf<<std::endl;
 
  ret = SSL_write (ssl, "I hear you.", strlen("I hear you.")); 
  if( ret == -1 ) {
    std::cout<<"SSL_write error."<<std::endl;
    return -1;
  }

  close(accept_socket);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
  return 0;
}
