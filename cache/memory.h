#ifndef CACHE_MEMORY_H_
#define CACHE_MEMORY_H_

#include <stdint.h>
#include "storage.h"

class Memory: public Storage {
public:
	Memory(int size = 0);
	~Memory();

	// Main access process
	void HandleRequest(uint addr, int bytes, int read, char *content, int &hit, int &time);
	void Load(uint addr, char c);
	void Print();			// for debug

private:
	// Memory implement
	char *memory;			// if memory is fake, memory == nullptr

	int size_;

	DISALLOW_COPY_AND_ASSIGN(Memory);
};

#endif //CACHE_MEMORY_H_ 
