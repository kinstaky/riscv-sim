CC = g++
DEFINE = -DFIFO -DQ3 -DSTREAMBUFFER

all: sim

sim: main.o cache.o memory.o
	$(CC) $(DEFINE) -o $@ $^

main.o: main.cc cache.h
	$(CC) $(DEFINE) -c $^

cache.o: cache.cc cache.h def.h
	$(CC) $(DEFINE) -c $^

memory.o: memory.cc memory.h
	$(CC) $(DEFINE) -c $^

.PHONY: clean

clean:
	rm -rf sim *.o
