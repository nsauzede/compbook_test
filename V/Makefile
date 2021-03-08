CFLAGS:=-std=c11 -g -static
VC:=v
TARGET:=9cc

all: $(TARGET)

test: $(TARGET)
	./test.sh

check: $(TARGET)
	./$(TARGET) 123 > tmp.v
	$(VC) -o tmp tmp.v
	./tmp ; echo $$?

clean:
	$(RM) $(TARGET) tmp.v *~ tmp*

.PHONY: check test clean
