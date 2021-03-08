CFLAGS:=-std=c11 -g
LDFLAGS+=-static
CFLAGS+=-O0
SRCS:=$(wildcard *.c)
OBJS:=$(SRCS:.c=.o)

TARGET:=9cc

all: $(TARGET)

9cc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test: $(TARGET)
	./test.sh

check: $(TARGET)
	./$(TARGET) 'main(){return 123;}' > tmp.s
	$(CC) -o tmp tmp.s
	./tmp ; echo $$?

clean:
	$(RM) $(TARGET) *.o *~ tmp*

.PHONY: check test clean
