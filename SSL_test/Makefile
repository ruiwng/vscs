include  ../Make.defines

PROGS = server client

all : $(PROGS)

server : server.o 
	$(CC) $(CPPFLAGS) -o $@ server.o $(LIBS)

client : client.o 
	$(CC) $(CPPFLAGS) -o $@ client.o $(LIBS)

clean :
	rm -f $(PROGS) *.o
