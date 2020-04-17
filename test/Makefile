GCC = riscv64-unknown-elf-gcc
CFLAGS = -Wa,-march=rv64i -static -Wl,--no-relax

all: hello add qsort nn mul-div simple-fuction ack matrix

hello: hello.c lib.c
	$(GCC) $(CFLAGS) -o $@.out $^

add: add.c lib.c
	$(GCC) $(CFLAGS) -o $@.out $^

qsort: qsort.c lib.c
	$(GCC) $(CFLAGS) -o $@.out $^

nn: nn.c lib.c
	$(GCC) -Wa,-march=rv64im -static -Wl,--no-relax -o $@.out $^

mul-div: mul-div.c lib.c
	$(GCC) $(CFLAGS) -o $@.out $^

simple-fuction: simple-fuction.c lib.c
	$(GCC) $(CFLAGS) -o $@.out $^

ack: Ackermann.c lib.c
	$(GCC) $(CFLAGS) -o $@.out $^

matrix: matrix.c lib.c
	$(GCC) -Wa,-march=rv64im -static -Wl,--no-relax -o $@.out $^

clean:
	rm *.out
