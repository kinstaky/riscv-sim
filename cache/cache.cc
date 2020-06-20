#include "cache.h"
#include "def.h"
// #include <cstdio>

#define WIDTH 32

int intLog2(int x) {
    if (x == 0) {
        fprintf(stderr, "intLog2: zero\n");
        return -1;
    }
    for (int i = 0; i != WIDTH; ++i) {
        if (x >> i == 1) {
            return i;
        }
    }
    fprintf(stderr, "intLog2: not the expotion of 2\n");
    return -1;
}



Cache::Cache() {
    lower_ = nullptr;
    sets = nullptr;
}

Cache::~Cache() {
    for (int i = 0; i != 1<<setBits; ++i) {
        delete sets[i];
    }
    if (sets) {
        delete[] sets;
    }
}

void Cache::SetConfig(CacheConfig cc) {
    if (sets != nullptr) {
        for (int i = 0; i != 1<<setBits; ++i) {
            delete sets[i];
        }
        delete[] sets;
    }
    config_.size = cc.size;
    config_.associativity = cc.associativity;
    config_.setNum = cc.setNum;
    config_.writeThrough = cc.writeThrough;
    config_.writeAllocate = cc.writeAllocate;
    config_.fake = cc.fake;
    sets = new CacheSet*[config_.setNum];
    int blockSize = config_.size / config_.associativity / cc.setNum;
    blockBits = intLog2(blockSize);
    setBits = intLog2(config_.setNum);
    labelBits = WIDTH - blockBits - setBits;
    for (int i = 0; i != config_.setNum; ++i) {
        sets[i] = new CacheSet(config_.associativity, labelBits, blockBits, config_.fake);
    }
#ifdef STREAMBUFFER
    config_.streamBuffer = cc.streamBuffer;
    if (config_.streamBuffer) {
    	streamBuffer = new StreamBuffer(config_.fake, blockSize);
    	StorageLatency sl;
    	sl.bus_latency = 0;
    	sl.hit_latency = 1;					// only 1 cycle
    	streamBuffer->SetLatency(sl);
    	streamBuffer->SetLower(lower_);
    	streamBuffer->SetStats(stats_);
    }
#endif
}

void Cache::GetConfig(CacheConfig &cc) {
    cc.size = config_.size;
    cc.associativity = config_.associativity;
    cc.setNum = config_.setNum;
    cc.writeThrough = config_.writeThrough;
    cc.writeAllocate = config_.writeAllocate;
    cc.fake = config_.fake;
#ifdef STREAMBUFFER
    cc.streamBuffer = config_.streamBuffer;
#endif
}


void Cache::GetStreamBufferStats(StorageStats &ss) {
	if (config_.streamBuffer) {
		streamBuffer->GetStats(ss);
	}
	return;
}

void Cache::request(uint addr, int bytes, int read, char *content, int &hit, int &time) {
    hit = 0;
    time = 0;
    // Bypass?
    int set, label, offset;
    if (!BypassDecision()) {
        PartitionAlgorithm(addr, label, set, offset);
        // Miss?
        if (ReplaceDecision(label, set)) {
            // Choose victim
            stats_.miss_num++;
            if (read || !config_.writeAllocate) {
                if (ReplaceAlgorithm(set, time)) {
                    stats_.replace_num++;
                }
            }
        } else {
            // write through and write
            if (read == 0 && config_.writeThrough == 1) {
                hit = 1;
                int lower_hit, lower_time;
                lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
                time += latency_.bus_latency + lower_time;
                stats_.access_time += time;
                sets[set]->Request(offset, label, bytes, read, content);
                stats_.access_counter++;
                return;
            }
            // return hit & time
            hit = 1;
            time += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += time;
            sets[set]->Request(offset, label, bytes, read, content);
            stats_.access_counter++;
            return;
        }
    }

    int lower_hit, lower_time;
    char *buffer = new char[1<<blockBits];
    // Prefetch?
    if ((read || !config_.writeAllocate) && PrefetchDecision(addr-offset, 1<<blockBits, buffer, lower_hit, lower_time)) {	// non-write allocate write
  		// lower_hit indicates which buffer hit
    	// fetch new line
    	time += latency_.bus_latency + lower_time;
    	sets[set]->NewLine(label, buffer);
        if (!read && config_.writeThrough) {
        	lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
            time += lower_time;
        }
        stats_.access_time += time;
        stats_.access_counter++;
        stats_.prefetch_num++;
        sets[set]->Request(offset, label, bytes, read, content);
    } else {
        hit = 0;
        // write allocate
        if (!read && config_.writeAllocate) {
            lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
            time += latency_.bus_latency + lower_time;
            stats_.access_time += time;
            stats_.access_counter++;
            return;
        }
        // non write allocate
        // fetch new line
        lower_->HandleRequest(addr-offset, 1<<blockBits, 1, buffer, lower_hit, lower_time);
        sets[set]->NewLine(label, buffer);

        time += latency_.bus_latency + lower_time;
        // write through
        if (!read && config_.writeThrough) {
            lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
            time += lower_time;
        }
        stats_.access_time += time;
        stats_.access_counter++;
        stats_.fetch_num++;
        sets[set]->Request(offset, label, bytes, read, content);

#ifdef STREAMBUFFER
        if (config_.streamBuffer) {
        	streamBuffer->StrideVerrify(addr-offset);
        }
#endif
    }
    delete[] buffer;
}


void Cache::HandleRequest(uint addr, int bytes, int read, char *content, int &hit, int &time) {
    hit = 0;
    time = 0;
    uint blockSize = 1 << blockBits;
    uint alignAddr = (addr / blockSize) << blockBits;
    if (addr + bytes <= alignAddr + blockSize) {
        request(addr, bytes, read, content, hit, time);
        return;
    }

    int pbytes = 0;                     // processed bytes
    int subtime, subhit;
    if (addr != alignAddr) {
        pbytes += blockSize - (addr - alignAddr);
        request(addr, pbytes, read, content, subhit, subtime);
        time += subtime;
        hit &= subhit;
    }
    while (pbytes + blockSize <= bytes) {
        request(addr+pbytes, blockSize, read, content+pbytes, subhit, subtime);
        time += subtime;
        hit &= subhit;
        pbytes += blockSize;
    }
    if (pbytes < addr+bytes) {
        request(addr+pbytes, addr+bytes-pbytes, read, content+pbytes, subhit, subtime);
        time += subtime;
        hit &= subhit;
        pbytes = addr + bytes;
    }
    return;
}


int Cache::BypassDecision() {
    return FALSE;
}

void Cache::PartitionAlgorithm(uint addr, int &label, int &set, int &offset) {
    uint debugAddr = addr;
    offset = addr & ((1 << blockBits) - 1);
    addr = addr >> blockBits;
    set = addr & ((1 << setBits) - 1);
    addr = addr >> setBits;
    label = addr & ((1 << labelBits) - 1);
    //fprintf(stderr, "addr %u, label %d, set %d, offset %d\n", debugAddr, label, set, offset);
}

int Cache::ReplaceDecision(int label, int set) {
    return !(sets[set]->Label(label));
}

bool Cache::ReplaceAlgorithm(int set, int &time){
    return sets[set]->Replace(config_.writeThrough, lower_, set, time);
}

bool Cache::PrefetchDecision(uint addr, int bytes, char *content, int &hit, int &time) {
#ifdef STREAMBUFFER
	if (!config_.streamBuffer) return false;
	streamBuffer->HandleRequest(addr, bytes, 1, content, hit, time);
	if (hit) return true;
	return false;
#else
	return FALSE;
#endif
}


void Cache::Print() {
    printf("\n--------------------------------------------------\nCache:\n");
    for (int i = 0; i != 1<<setBits; ++i) {
        printf("Set %d:\n", i);
        sets[i]->Print();
    }
    if (config_.streamBuffer) {
    	streamBuffer->Print();
    }
    lower_->Print();
}




CacheSet::CacheSet(int LineNum, int LabelBits, int BlockBits, bool fake_) {
    lineNum = LineNum;
    lines = new CacheLine*[LineNum];
    for (int i = 0; i != lineNum; ++i) {
        lines[i] = new CacheLine(LabelBits, BlockBits, fake_);
    }
#ifdef FIFO
    replaceStack = 0;
#endif
#ifdef CLOCK
    replaceClock = 0;
#endif
}

CacheSet::~CacheSet() {
    for (int i = 0; i != lineNum; ++i) {
        delete lines[i];
    }
    delete[] lines;
}

void CacheSet::Request(uint offset, int label, int bytes, int read, char *content) {
    for (int i = 0; i != lineNum; ++i) {
        if (lines[i]->Valid() && lines[i]->Label(label)) {
            lines[i]->Request(offset, bytes, read, content);
            return;
        }
    }
    fprintf(stderr, "CacheSet::Request Error: no valid or label line, label %d\n", label);
    return;
}

bool CacheSet::Label(int label) {
    for (int i = 0; i != lineNum; ++i) {
        if (lines[i]->Valid() && lines[i]->Label(label)) {
            return true;
        }
    }
    return false;
}

bool CacheSet::Replace(int writeThrough, Storage* lower, int set, int &time) {
    for (int i = 0; i!= lineNum; ++i) {
        if (!lines[i]->Valid()) {
            return false;
        }
    }
#ifdef FIFO
    if (!writeThrough) {                // write back
        lines[replaceStack]->WriteBack(lower, set, time);
    }
    lines[replaceStack]->InValid();
    replaceStack++;
    if (replaceStack == lineNum) {
        replaceStack = 0;
    }
#endif
#ifdef CLOCK
    while (true) {
        if (lines[replaceClock]->Clock()) {
            break;
        }
        replaceClock++;
        if (replaceClock == lineNum) {
            replaceClock = 0;
        }
    }
    if (!writeThrough) {
        lines[replaceClock]->WriteBack(lower, set, time);
    }
    lines[replaceClock]->InValid();
    replaceClock++;
    if (replaceClock == lineNum) {
        replaceClock = 0;
    }
#endif
    return true;
}

void CacheSet::NewLine(int label, char *content) {
    for (int i = 0; i != lineNum; ++i) {
        if (!lines[i]->Valid()) {
            lines[i]->Write(label, content);

            return;
        }
    }
    fprintf(stderr, "CacheSet::NewLine Error: no valid lines\n");
    return;
}

void CacheSet::Print() {
    for (int i = 0; i != lineNum; ++i) {
        printf("%4d:\t", i);
        lines[i]->Print();
    }
}



CacheLine::CacheLine(int LabelBits, int BlockBits, bool fake_) {
    fake = fake_;
    if (!fake) {
        block = new char[1<<BlockBits];
    }
    blockBits = BlockBits;
    labelBits = LabelBits;
    valid = false;
}

CacheLine::~CacheLine() {
    if (!fake) {
        delete[] block;
    }
}

bool CacheLine::Valid() {
    return valid;
}

void CacheLine::InValid() {
    valid = false;
    return;
}


bool CacheLine::Label(int l) {
    return l == label;
}

void CacheLine::Request(uint offset, int bytes, int read, char *content) {
    if (read) {
        visitFlag = 1;
        if (fake) return;
        for (int i = 0; i != bytes; ++i) {
            content[i] = block[offset+i];
        }
    } else {
        dirtyFlag = 1;
        if (fake) return;
        for (int i = 0; i != bytes; ++i) {
            block[offset+i] = content[i];
        }
    }
    return;
}

void CacheLine::WriteBack(Storage *lower, int Set, int &time) {
    if (fake) {
        return;
    }
    if (dirtyFlag == 0) {
        return;
    }
    int setBits = WIDTH - blockBits - labelBits;
    uint addr = (label << (blockBits + setBits)) | (Set << blockBits);
    int lower_hit, lower_time;
    lower->HandleRequest(addr, 1<<blockBits, 0, block, lower_hit, lower_time);
    time += lower_time;
    return;
}

void CacheLine::Write(int Label, char *content) {
    label = Label;
    valid = true;
    visitFlag = 0;
    dirtyFlag = 0;
    if (fake) {
        return;
    }
    Request(0, 1<<blockBits, 0, content);
    dirtyFlag = 0;
}


#ifdef CLOCK
/*
        before              after
    dirty   visit       dirty   visit
    0       0           evict
    0       1           0		0
    1		1			1		0
    1		0			1		-1
    1		-1			evict
*/
bool CacheLine::Clock() {
    if (dirtyFlag + visitFlag == 0) {
        return true;
    }
    visitFlag -= 1;
    return false;
}
#endif


void CacheLine::Print() {
    printf("%d\t", valid);
    printf("%d %d\t", visitFlag, dirtyFlag);
    if (!fake && valid) {
        printf("%d\t", label);
        for (int i = 0; i != 1<<blockBits; ++i) {
            printf("%c", block[i]);
        }
    }
    printf("\n");
}


StreamBuffer::StreamBuffer(bool fake_, int BlockSize) {
	fake = fake_;
	blockSize = BlockSize;
	setNum = 8;
	historyNum = 8;
	lineNum = 4;
	historyTop = 0;
	for (int i = 0; i != historyNum; ++i) {
		history[i].valid = false;
	}
	for (int i = 0; i != setNum; ++i) {
		sets[i].valid = false;
		sets[i].stamp = 0;
		sets[i].top = 0;
		for (int j = 0; j != lineNum; ++j) {
			sets[i].lines[j].valid = false;
			if (!fake) {
				sets[i].lines[j].block = new char[blockSize];
			}
		}
	}
}

StreamBuffer::~StreamBuffer() {
	if (!fake) {
		for (int i = 0; i != setNum; ++i) {
			for (int j = 0; j != lineNum; ++j) {
				delete[] sets[i].lines[j].block;
			}
		}
	}
}


void StreamBuffer::HandleRequest(uint addr, int bytes, int read, char *content, int &hit, int &time) {
	time = 0;
	hit = 0;
	stats_.access_counter++;
	for (int i = 0; i != setNum; ++i) {
		if (sets[i].valid) {
			if (sets[i].lines[sets[i].top].valid && sets[i].lines[sets[i].top].addr == addr) {
				hit = 1;
				time = latency_.hit_latency + latency_.bus_latency;
				if (!fake) {
					if (read) {
						for (int j = 0; j != bytes; ++j) {
							content[j] = sets[i].lines[sets[i].top].block[j];
						}
					} else {
						fprintf(stderr, "Stream buffer Error: write\n");
					}
					// prefetch the next [lineNum] blocks
					char *buffer = new char[blockSize];
					int lower_hit, lower_time;
					lower->HandleRequest(addr+lineNum*blockSize, blockSize, 1, buffer, lower_hit, lower_time);
					if (!lower_hit) {
						sets[i].lines[sets[i].top].valid = false;
					} else {
						stats_.fetch_num++;
					}
					for (int j = 0; j != blockSize; ++j) {
						sets[i].lines[sets[i].top].block[j] = buffer[j];
					}
					delete[] buffer;
				} else {
					int lower_hit, lower_time;
					lower->HandleRequest(addr+lineNum*blockSize, 0, 1, nullptr, lower_hit, lower_time);
					if (!lower_hit) {
						sets[i].lines[sets[i].top].valid = false;
					} else {
						stats_.fetch_num++;
					}
				}
				// set the new addr
				sets[i].lines[sets[i].top].addr = addr + lineNum*blockSize;
				// change the top
				sets[i].top++;
				if (sets[i].top == lineNum) sets[i].top = 0;
				// change the stamp for replace
				for (int j = 0; j != setNum; ++j) {
					sets[j].stamp = sets[j].stamp >> 1;
				}
				sets[i].stamp += 128;
				return;
			}
		}
	}
	// stream buffer miss
	stats_.miss_num++;
	return;
}

void StreamBuffer::StrideVerrify(uint addr) {
	// record the miss address
	history[historyTop].valid = true;
	history[historyTop].addr = addr + blockSize;
	historyTop++;
	if (historyTop == historyNum) historyTop = 0;
	// find the continous miss
	for (int i = 0; i != historyNum; ++i) {
		if (history[i].valid && history[i].addr == addr) {
			// allocate a new buffer
			// find a invalid set, if exist
			for (int j = 0; j != setNum; ++j) {
				if (!sets[j].valid) {
					sets[j].valid = true;
					sets[j].stamp = 128;
					sets[j].top = 0;
					for (int k = 0; k != lineNum; ++k) {
						sets[j].lines[k].valid = true;
						sets[j].lines[k].addr = addr + blockSize*(k+1);
						if (!fake) {
							int lower_hit, lower_time;
							char *buffer = new char[blockSize];
							lower->HandleRequest(addr+blockSize*(k+1), blockSize, 1, buffer, lower_hit, lower_time);
							if (lower_hit) {
								for (int p = 0; p != blockSize; ++p) {
									sets[j].lines[k].block[p] = buffer[p];
								}
								stats_.fetch_num++;
							} else {
								sets[j].lines[k].valid = false;
							}
							delete[] buffer;
						} else {			// fake, read nothing, but to keep the stats consist
							int lower_hit, lower_time;
							lower->HandleRequest(addr+blockSize*(k+1), 0, 1, nullptr, lower_hit, lower_time);
							if (!lower_hit) {
								sets[j].lines[k].valid = false;
							} else {
								stats_.fetch_num++;
							}
						}
					}
					return;
				}
			}
			// replace one
			stats_.replace_num++;
			int minStamp = sets[0].stamp;
			int minIndex = 0;
			for (int j = 1; j != setNum; ++j) {
				if (minStamp > sets[j].stamp) {
					minStamp = sets[j].stamp;
					minIndex = j;
				}
			}
			sets[minIndex].stamp = 128;
			sets[minIndex].top = 0;
			for (int k = 0; k != lineNum; ++k) {
				sets[minIndex].lines[k].valid = true;
				sets[minIndex].lines[k].addr = addr + blockSize*(k+1);
				if (!fake) {
					int lower_hit, lower_time;
					char *buffer = new char[blockSize];
					lower->HandleRequest(addr+blockSize*(k+1), blockSize, 1, buffer, lower_hit, lower_time);
					if (lower_hit) {
						for (int p = 0; p != blockSize; ++p) {
							sets[minIndex].lines[k].block[p] = buffer[p];
						}
					}
					delete[] buffer;
				}
			}
		}
	}
}


void StreamBuffer::Print() {
	printf("\n--------------------------------------------------\nStreamBuffer:\n");
	for (int i = 0; i != setNum; ++i) {
		printf("Set %d, valid %d, stamp %d, top %d\n", i, sets[i].valid, sets[i].stamp, sets[i].top);
		if (!sets[i].valid) {
			continue;
		}
		for (int j = 0; j != lineNum; ++j) {
			printf("\tLine %d:\t%d\t%u", j, sets[i].lines[j].valid, sets[i].lines[j].addr);
			if (!fake) {
				printf("\t");
				for (int k = 0; k != blockSize; ++k) {
					printf("%c", sets[i].lines[j].block[k]);
				}
			}
			printf("\n");
		}
	}
	printf("History miss address:\n");
	for (int i = 0; i != historyNum; ++i) {
		int index = historyTop + i + 1;
		if (index > historyNum) {
			index -= historyNum;
		}
		printf("%d", history[i].valid);
		if (history[i].valid) {
			printf("\t%u", history[i].addr);
		}
		printf("\n");
	}
}
