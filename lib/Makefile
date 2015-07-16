include  ../Make.defines

LIB_OBJS = error_log.o readn.o writen.o daemonize.o client_info.o

all: ${LIB_OBJS}
	ar rv ${LIBCSD} $?

clean:
	rm -f $(LIBCSD) ${CLEANFILES}
