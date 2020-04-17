MAKE = make

all:
	cd src; $(MAKE); cd ../;
	cd test; $(MAKE); cd ../;

hello:
	bin/sim.out test/$@.out

add:
	bin/sim.out test/$@.out

mul-div:
	bin/sim.out test/$@.out

simple-fuction:
	bin/sim.out test/$@.out

qsort:
	bin/sim.out test/$@.out

nn:
	bin/sim.out test/$@.out

ack:
	bin/sim.out test/$@.out

matrix:
	bin/sim.out test/$@.out


hello-s:
	bin/sim.out -s test/hello.out

add-s:
	bin/sim.out -s test/add.out

mul-div-s:
	bin/sim.out -s test/mul-div.out

simple-function-s:
	bin/sim.out -s test/simple-function.out

qsort-s:
	bin/sim.out -s test/qsort.out

nn-s:
	bin/sim.out -s test/nn.out

ack-s:
	bin/sim.out -s test/ack.out

matrix-s:
	bin/sim.out -s test/matrix.out
