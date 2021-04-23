CFLAGS:=-std=c11 -g
LDFLAGS+=-static
CFLAGS+=-O0
 SRCS:=$(wildcard *.c)
#SRCS:=codegen.c main.c parse.c tokenize.c type.c
OBJS:=$(SRCS:.c=.o)

TARGET:=chibicc

all: $(TARGET)

chibicc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test: $(TARGET)
	./test_v.sh
	./test.sh

check: $(TARGET)
	./$(TARGET) 123 > tmp.s
	$(CC) -o tmp tmp.s
	./tmp ; echo $$?

clean:
	$(RM) $(TARGET) *.o *~ tmp*

.PHONY: check test clean
