CFLAGS:=-std=c11 -g -static

TARGET:=9cc

all: $(TARGET)

test: $(TARGET)
	./test.sh

check: $(TARGET)
	./$(TARGET) 123 > tmp.s
	$(CC) -o tmp tmp.s
	./tmp ; echo $$?

clean:
	$(RM) $(TARGET) *.o *~ tmp*

.PHONY: check test clean
