CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

tcc: tcc.o
	$(CC) -o $@ $? $(LDFLAGS)

tcc: $(OBJS)
	$(CC) -o $@ $? $(LDFLAGS)

	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): tcc.h

test: tcc
	./test.sh

clean:
	rm -f tcc *.o *~ tmp*

.PHONY: test clean
