#include  "vscs.h" 
#include <iostream>

#define    SERVER_CERTIFICATE  "cacert.pem"
#define    SERVER_KEY          "privatekey.pem"

 int main( int argc, char* argv[] ) {
   int ret;

   SSL_CTX* ctx_client = ssl_client_init();
   SSL_CTX* ctx_server = ssl_server_init(SERVER_CERTIFICATE, SERVER_KEY);
   int client_socket, server_socket;
   client_socket = client_connect("127.0.0.1","5003");
   if( client_socket < 0  ) {
     std::cout<<"client_connect error."<<std::endl;
     return -1;
    }
   server_socket = server_listen("5004");
   SSL* ssl_cli = ssl_client(ctx_client, client_socket);

  char buf[4096];
  ret = SSL_write(ssl_cli, "Hello World!", strlen("Hello World!"));  
  if( ret == -1 ) {
    std::cout<<"SSL_write error."<<std::endl;
    return -1;
  }
	int accept_socket = accept(server_socket, NULL, NULL);
	SSL *ssl_ser = ssl_server(ctx_server, accept_socket);
  ret = SSL_read (ssl_ser, buf, sizeof(buf) - 1);  
  if( ret == -1 ) {
    std::cout<<"SSL_read error."<<std::endl;
    return -1;
  }
  buf[ret] = '\0';
  printf("client: %s\n", buf);
  return 0;
}
