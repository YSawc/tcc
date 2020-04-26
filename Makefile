CFLAGS=-std=c11 -g -static -fno-common

tcc: main.o
	$(CC) -o $@ $? $(LDFLAGS)

test: tcc
	./test.sh

clean:
	rm -f tcc *.o *~ tmp*

.PHONY: test clean
