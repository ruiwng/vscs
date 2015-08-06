include  ../Make.defines

PROGS = sha_test

all : $(PROGS) 

sha_test : sha_test.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LIBS)

clean :
	rm -f $(PROGS) $(CLEANFILES)


