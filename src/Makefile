GCC = g++
CFLAGS = -I ../include

sim.out: Simulation.cpp
	$(GCC) $(CFLAGS) -o ../bin/$@ $^

clean:
	rm ../bin/sim.out