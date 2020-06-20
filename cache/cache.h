#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"

class Cache;

typedef struct CacheConfig_ {
    int size;
    int associativity;
    int setNum;            // Number of cache sets
    int writeThrough;      // 0|1 for back|through
    int writeAllocate;     // 0|1 for no-alc|alc
    bool fake;
#ifdef STREAMBUFFER
    bool streamBuffer;
#endif
} CacheConfig;


class CacheLine {
public:
    CacheLine(int LabelBits, int BlockBits, bool fake_= true);
    ~CacheLine();
    bool Valid();														// return valid
    void InValid();														// set valid to false
    bool Label(int l);													// whether the label is l?
    void Request(uint offset, int bytes, int read, char *content);		// request handler
    void WriteBack(Storage *lower, int Set, int &time);                 // write back to the lower storage
    void Write(int Label, char *content);                               // write the whole block
#ifdef CLOCK
    bool Clock();
#endif
    void Print();														// for debug
private:
    bool valid;
    int label;
    int labelBits;
    int blockBits;
    char *block;
    bool fake;
    int dirtyFlag;
    int visitFlag;
};


class CacheSet {
public:
    CacheSet(int LineNum, int LabelBits, int BlockBits, bool fake_ = true);
    ~CacheSet();
    void Request(uint offset, int label, int bytes, int read, char *content);   // request handler
    bool Label(int label);                              						// whether contains this label
    bool Replace(int writeThrough, Storage *lower, int set, int &time);     	// evict one line
    void NewLine(int label, char *content);             						// add content to empty line
    void Print();                                       						// for debug
private:
    CacheLine **lines;
    int lineNum;
#ifdef FIFO
    int replaceStack;
#endif
#ifdef CLOCK
    int replaceClock;
#endif
};

struct BufferLine {
    bool valid;
    uint addr;
    char *block;
};

struct BufferSet {
    bool valid;
    int stamp;
    int top;
    BufferLine lines[4];
};

struct HistoryEntry {
    uint addr;
    bool valid;
};


class StreamBuffer: public Storage {
public:
    StreamBuffer(bool fake_, int BlockSize);
    ~StreamBuffer();
    void SetLower(Storage *ll) {lower = ll;}
    void HandleRequest(uint addr, int bytes, int read, char *content, int &hit, int &time);
    void StrideVerrify(uint addr);
    void Print();
private:
    DISALLOW_COPY_AND_ASSIGN(StreamBuffer);
    Storage *lower;
    bool fake;
    int blockSize;

    HistoryEntry history[8];
    int historyTop;

    int setNum;
    int historyNum;
    int lineNum;
    BufferSet sets[8];
};



class Cache: public Storage {
public:
    Cache();
    ~Cache();

    // Sets & Gets
    void SetConfig(CacheConfig cc);
    void GetConfig(CacheConfig &cc);
    void SetLower(Storage *ll) { lower_ = ll; }
    // Main access process
    void HandleRequest(uint addr, int bytes, int read, char *content, int &hit, int &time);
    void GetStreamBufferStats(StorageStats &ss);
    void Print();           // for debug

private:
    // Bypassing
    int BypassDecision();
    // Partitioning
    void PartitionAlgorithm(uint addr, int &label, int &set, int &offset);
    // Replacement
    int ReplaceDecision(int label, int set);
    bool ReplaceAlgorithm(int set, int &time);
    // Prefetching
    bool PrefetchDecision(uint addr, int bytes, char *content, int &hit, int &time);

    void request(uint addr, int bytes, int read, char *content, int &hit, int &time);

    CacheConfig config_;
    Storage *lower_;
    DISALLOW_COPY_AND_ASSIGN(Cache);


    CacheSet **sets;
    int blockBits, setBits, labelBits;
#ifdef STREAMBUFFER
    StreamBuffer *streamBuffer;
#endif
};

#endif //CACHE_CACHE_H_ 
