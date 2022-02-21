CFLAGS:=-std=c11 -g
CFLAGS+=-O0
CFLAGS+=-D_POSIX_C_SOURCE=200809L
LDFLAGS+=-static
 SRCS:=$(wildcard *.c)
#SRCS:=codegen.c main.c parse.c tokenize.c type.c
OBJS:=$(SRCS:.c=.o)

TARGET:=chibicc

all: $(TARGET)

chibicc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test: $(TARGET)
	./test.sh

testv: $(TARGET)
	./test_v.sh

check: $(TARGET)
	echo 'int main(){return 123;}' > tmp.c.txt ; ./$(TARGET) tmp.c.txt > tmp.s
	$(CC) -o tmp tmp.s
	./tmp ; echo $$?

clean:
	$(RM) $(TARGET) *.o *~ tmp*

.PHONY: check test clean
