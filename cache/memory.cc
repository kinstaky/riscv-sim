#include "memory.h"
#include <iostream>

Memory::Memory(int size) {
	size_ = size;
	if (size == 0) {
		memory = nullptr;
	} else {
		memory = new char[size];
	}
}

Memory::~Memory() {
	if (memory != nullptr) {
		delete[] memory;
	}
}


void Memory::HandleRequest(uint addr, int bytes, int read, char *content, int &hit, int &time) {
	hit = 1;
	time = latency_.hit_latency + latency_.bus_latency;
	stats_.access_time += time;
	stats_.access_counter++;

  	if (memory != nullptr) {
  		if (read) {
  			for (int i = 0; i != bytes; ++i) {
  				content[i] = memory[addr+i];
  			}
  		} else {
  			for (int i = 0; i != bytes; ++i) {
  				memory[addr+i] = content[i];
  			}
  		}
  	}
  	return;
}

void Memory::Load(uint addr, char c) {
	memory[addr] = c;
	return;
}



void Memory::Print() {
	printf("\n--------------------------------------------------\nMemory:\n");
	for (int i = 0; i != size_; ++i) {
		printf("%c", memory[i]);
	}
	printf("\n");
	return;
}

