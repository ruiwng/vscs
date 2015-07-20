include  ../Make.defines

LIB_OBJS = client_info.o error_log.o readn.o writen.o daemonize.o client_connect.o server_listen.o ssl_init.o read_s.o ssl_client.o ssl_server.o ssl_readn.o ssl_writen.o error.o

all: $(LIB_OBJS)
	ar rv $(LIBVSCS) $?

clean:
	rm -f $(LIBVSCS) $(CLEANFILES)
