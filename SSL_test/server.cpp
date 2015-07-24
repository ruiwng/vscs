#include  "vscs.h"
#include  <iostream>

#define SERVER_PORT    5003


#define SERVER_CERTIFICATE   "cacert.pem"    
#define SERVER_KEY           "privatekey.pem"

 
int main( int argc, char* argv[] ) {
  int ret;

  SSL_CTX* ctx_server = ssl_server_init(SERVER_CERTIFICATE, SERVER_KEY);
  SSL_CTX* ctx_client = ssl_client_init();
  int listen_socket;
  int accept_socket;

  listen_socket = server_listen("5003");  
  if( listen_socket < 0 ) {
    std::cout<<"socket error."<<std::endl;
    return -1;
  }

    accept_socket = accept (listen_socket, NULL, NULL);
  if( accept_socket < 0  ) {
    std::cout<<"accept error."<<std::endl;
    return -1;
  }
  close(listen_socket);

  SSL *ssl = ssl_server(ctx_server, accept_socket);
  char     buf [4096];

  ret = SSL_read (ssl, buf, sizeof(buf) - 1);    
  if( ret == -1 ) {
    std::cout<<"SSL_read error."<<std::endl;
    return -1;
  }
  buf[ret] = '\0';
  printf("%s\n", buf);
  int client_socket = client_connect("127.0.0.1", "5004");
  SSL *ssl_cli = ssl_client(ctx_client, client_socket);
  ret = SSL_write (ssl_cli, "I hear you.", strlen("I hear you.")); 
  if( ret == -1 ) {
    std::cout<<"SSL_write error."<<std::endl;
    return -1;
  }

  return 0;
}
