include  ../Make.defines

PROGS = vscs_master

all : $(PROGS)

vscs_master : master_server.o client_info.o master_config.o command_parse.o msg_queue.o
	$(CC) -o $@ client_info.o master_config.o master_server.o msg_queue.o command_parse.o $(LIBS)

clean :
	rm -f $(PROGS) $(CLEANFILES)
