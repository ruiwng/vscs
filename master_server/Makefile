include  ../Make.defines

PROGS = master_server

all : $(PROGS)

master_server : client_info.o master_config.o master_server.o msg_queue.o 
	$(CC) $(CFLAGS) -o $@ client_info.o master_config.o master_server.o msg_queue.o $(LIBS)

clean :
	rm -f $(PROGS) $(CLEANFILES)
