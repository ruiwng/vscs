include  ../Make.defines

PROGS = press_test

all : $(PROGS)

press_test : press_test.o
	$(CC) $(FLAGS) -o $@ $^

clean :
	rm -f $(CLEANFILES) $(PROGS)

