GCC = g++
CFLAGS = -I ../include
DEFINE = -DLAB3 -DFIFO

sim.out: Simulation.cpp cache.o memory.o
	$(GCC) $(CFLAGS) $(DEFINE) -o ../bin/$@ $^

cache.o: ../cache/cache.cc ../cache/cache.h  ../cache/def.h
	$(GCC) $(CFLAGS) $(DEFINE) -c $^

memory.o: ../cache/memory.cc ../cache/memory.h
	$(GCC) $(CFLAGS) $(DEFINE) -c $^

clean:
	rm ../bin/sim.out