DIRS = lib client slave_server master_server

all :
	for i in $(DIRS); do \
		(cd $$i && echo "making $$i" && $(MAKE) ) || exit 1; \
	done

clean:
	for i in $(DIRS); do \
		(cd $$i && echo "making $$i" && $(MAKE) clean) || exit 1; \
	done



