CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(filter-out tests.c, $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

default: clean tcc test_c

tcc: $(OBJS)
	$(CC) -o $@ $? $(LDFLAGS)

$(OBJS): tcc.h

test: tcc
	./tcc tests > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

test_c: tcc
	./tcc tests.c > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

signle_test: tcc
	./tcc single_test > tmp.s
	gcc -static -o tmp tmp.s
	./tmp
	sed -i '/^.global main$$/,$$!d' tmp.s

old_test: tcc
	./old_test.sh

clean:
	rm -f tcc *.o *~ tmp*

.PHONY: test clean
