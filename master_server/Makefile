include  ../Make.defines

PROGS = vscs_master

all : $(PROGS)

vscs_master : master_server.o client_info.o master_config.o command_parse.o msg_queue.o cluster_status.o
	$(CC) -o $@ $^ $(LIBS)

clean :
	rm -f $(PROGS) $(CLEANFILES)
