include  ../Make.defines

PROGS = vscs_client

all: $(PROGS)

vscs_client: client_config.o command_parse.o transmit_file.o client.o
	$(CC) $(CFLAGS) -o $@ client_config.o command_parse.o transmit_file.o client.o $(LIBS)

clean:
	rm -f $(PROGS) $(CLEANFILES)


