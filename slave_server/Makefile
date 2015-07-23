include  ../Make.defines

PROGS = vscs_slave

all: $(PROGS)

vscs_slave : command_parse.o master_connect.o slave_server.o slave_config.o
	${CC} -o $@ command_parse.o master_connect.o slave_server.o slave_config.o ${LIBS}

clean:
	rm -f $(PROGS) ${CLEANFILES}

