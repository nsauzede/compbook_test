CFLAGS:=-std=c11 -g
CFLAGS+=-O0
CFLAGS+=-D_POSIX_C_SOURCE=200809L
LDFLAGS+=-static
SRCS:=$(wildcard *.c)
#SRCS:=codegen.c main.c parse.c tokenize.c type.c
OBJS:=$(SRCS:.c=.o)

TEST_SRCS:=$(wildcard test/*.c)
TESTS:=$(TEST_SRCS:.c=.exe)

TARGET:=chibicc

CC0:=$(CC)
CCT:=$(CC)

all: $(TARGET)

chibicc: $(OBJS)
	$(CC0) -o $@ $^ $(LDFLAGS)

test0: $(TARGET)
	echo 'int main(){return 123;}' > tmp.c.txt ; ./$(TARGET) -S -o tmp.s -x c tmp.c.txt
	echo 'int printf();int main(){printf("Hello \"World\"\\n");return 123;}' > tmp.c.txt ; ./$(TARGET) -S -o tmp.s -x c tmp.c.txt
	$(CC) -o tmp tmp.s
	./tmp ; echo $$?

test/%.exe: test/%.c
#	$(CC) -o- -E -P -C test/$*.c | $(CCT) -S -o test/$*.s -x c - && $(CC) -o $@ test/$*.s
	$(CC) $(CFLAGS) -E -P -C -o test/$*.i test/$*.c && $(CCT) $(CFLAGS) -S -o test/$*.s -x c test/$*.i && $(CC) -o $@ test/$*.s -x c test/common

#	$(CC) -E -P -C -o test/$*.i test/$*.c && $(CCT) -o $@ -x c test/$*.i

#	$(CC) -o- -E -P -C test/$*.c | $(CCT) -fno-stack-protector -O0 -o $@ -x c - -x c test/common
#	$(CC) -o- -E -P -C test/$*.c | $(CCT) -fno-stack-protector -O0 -o $@ -x c -
#	$(CC) -o- -E -P -C test/$*.c | $(CCT) -o $@ -x c -

test1_: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done

test1:
	$(MAKE) clean test1_ CCT=gcc
#	$(MAKE) clean test1_ CCT=tcc
	$(MAKE) clean all test1_ CCT=./chibicc

test2: $(TARGET)
	./test.sh

check: test0
test: check test1 test2

clean:
	$(RM) $(TARGET) *.o *~ tmp*
	$(RM) test/*.s test/*.i test/*.exe

.PHONY: check test test2 clean
