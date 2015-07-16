include  ../Make.defines

all: slave_server

slave_server:command_parse.o master_connect.o slave_server.o
	${CC} ${CFLAGS} -o $@ command_parse.o master_connect.o slave_server.o ${LIBS}

clean:
	rm -f slave_server ${CLEANFILES}

