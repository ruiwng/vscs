include  ../Make.defines

PROGS = db_test

all: ${PROGS}

db_test : db_test.o 
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}



