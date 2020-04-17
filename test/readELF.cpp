#include "../include/elfio/elfio.hpp"
#include <cstdio>

#define MAXADDR 100000000

char memory[MAXADDR] = {0};

void loadMemory(ELFIO::elfio *reader) {
	ELFIO::Elf_Half seg_num = reader->segments.size();
	for (int i = 0; i != seg_num; ++i) {
		const ELFIO::segment *pseg = reader->segments[i];

		uint64_t fullmemsz = pseg->get_memory_size();
		uint64_t fulladdr = pseg->get_virtual_address();

		if (fulladdr + fullmemsz > MAXADDR) {
			fprintf(stderr, "ELF file too big\n");
			exit(-1);
		}

		uint32_t filesz = pseg->get_file_size();
		uint32_t memsz = pseg->get_memory_size();
		uint32_t addr = (uint32_t)pseg->get_virtual_address();


		for (uint32_t p = addr; p < addr + memsz; ++p) {
			if (p < addr + filesz)
				memory[p] = pseg->get_data()[p-addr];
		}
	}
}


int main()
{
	//解析elf文件
	const char elfFile[] = "../test/add.out";
	ELFIO::elfio reader;
	if (!reader.load(elfFile)) {
		fprintf(stderr, "Error loading ELF file %s\n", elfFile);
		return -1;
	}

	loadMemory(&reader);

	int pc = reader.get_entry();
	printf("pc = %x\n" ,reader.get_entry());

	for (int i = 0; i != 6256; ++i) {
		printf("%02x", (unsigned char)memory[i+pc]);
        if (i % 16 == 15) printf("\n");
        else if (i % 2 == 1) printf(" ");
	}

	return 0;
}