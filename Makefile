CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

default: tcc

tcc: $(OBJS)
	$(CC) -o $@ $? $(LDFLAGS)

$(OBJS): tcc.h

test: tcc
	./test.sh

clean:
	rm -f tcc *.o *~ tmp*

.PHONY: test clean
