#include  "vscs.h" 
#include <iostream>

 int main( int argc, char* argv[] ) {
   int ret;

   SSL_CTX* ctx = ssl_client_init();

   int client_socket;
   client_socket = client_connect("127.0.0.1","5003");
   if( client_socket < 0  ) {
     std::cout<<"client_connect error."<<std::endl;
     return -1;
   }

   SSL*     ssl = ssl_client(ctx, client_socket);

  char     buf [4096];
 ret = SSL_write (ssl, "Hello World!", strlen("Hello World!"));  
 printf("%d\n", ret);
  if( ret == -1 ) {
    std::cout<<"SSL_write error."<<std::endl;
    return -1;
  }
  ret = SSL_read (ssl, buf, sizeof(buf) - 1);  
  if( ret == -1 ) {
    std::cout<<"SSL_read error."<<std::endl;
    return -1;
  }
  buf[ret] = '\0';
 std::cout<<buf<<std::endl;
  SSL_shutdown(ssl);  /* send SSL/TLS close_notify */

  close(client_socket);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
  return 0;
}
