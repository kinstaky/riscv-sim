#include <iostream>
#include <fstream>
#include "cache.h"
#include "memory.h"

using std::cin;
using std::cout;
using std::endl;
using std::ofstream;


int main() {

    int hit, time;

#ifndef Q3
#ifdef Q0
    Memory m(128);
#endif
#ifdef Q1
    Memory m;
#endif
    Cache l1;
    l1.SetLower(&m);

    StorageStats s, gs;
    s.access_time = 0;
    s.access_counter = 0;
    s.miss_num = 0;
    s.replace_num = 0;
    s.fetch_num = 0;
    s.prefetch_num = 0;
    m.SetStats(s);
    l1.SetStats(s);

    StorageLatency ml;
    ml.bus_latency = 7;
    ml.hit_latency = 100;
    m.SetLatency(ml);

    StorageLatency ll;
    ll.bus_latency = 3;
    ll.hit_latency = 10;
    l1.SetLatency(ll);

    CacheConfig lc;

#ifdef Q0
    lc.fake = false;
    lc.size = 32;
    lc.associativity = 2;
    lc.setNum = 4;
    lc.writeThrough = 0;
    lc.writeAllocate = 0;
    lc.streamBuffer = true;
    l1.SetConfig(lc);

    char content[64];
    sprintf(content, "0123456789abcdef101112131415161718191a1b1c1d1e1f");
    // l1.HandleRequest(0, 48, 0, content, hit, time);
    // l1.Print();
    for (int i = 0; i != 12; ++i) {
        l1.HandleRequest(i<<2, 4, 0, content+(i<<2), hit, time);
        l1.Print();
    }
    for (int i = 0; i != 4; ++i) {
        l1.HandleRequest(i<<2, 4, 1, content+48+(i<<2), hit, time);
        l1.Print();
    }
    // l1.HandleRequest(0, 48, 1, content, hit, time);
    // l1.Print();
    // l1.HandleRequest(0, 12, 1, content+48, hit, time);
    // l1.Print();
    // l1.HandleRequest(28, 4, 0, content, hit, time);
    // l1.Print();
    // l1.HandleRequest(12, 4, 1, content+48, hit, time);
    // l1.Print();
    // l1.HandleRequest(28, 4, 1, content+52, hit, time);
    // l1.Print();
    cout << content << endl;

    l1.GetStats(s);
    cout << "Total access time " << s.access_time << " ns" << endl;
#endif




#ifdef Q1
    lc.fake = true;
    lc.size = 1<<15;
    lc.associativity = 8;
    lc.setNum = 32;
    lc.writeThrough = 1;
    lc.writeAllocate = 1;

    int read[33000];
    uint addr[33000];

    ofstream fout;
    fout.open("result.txt", ofstream::out | ofstream::trunc);
    int cnt, outputRate;
    char c;
    cin >> cnt;
    for (int i = 0; i != cnt; ++i) {
        cin >> c >> addr[i];
        read[i] = c == 'r' ? 1 : 0;
    }
    outputRate = 0;                 // 1 output miss rate, 0 output access time

    cout << "---------------------------------------------------" << endl;
    cout << "MissRate-BlockSize(32 sets and 8 lines):" << endl;
    for (int i = 0; i != 11; ++i) {
        lc.size = 1<<(15+i);
        l1.SetConfig(lc);
        l1.SetStats(s);

        for (int j = 0; j != cnt; ++j) {
            l1.HandleRequest(addr[j], 0, read[j], nullptr, hit, time);
        }
        l1.GetStats(gs);
        cout << "BlockSize " << (1<<(7+i)) << endl;
        cout << "   Total access_time " << gs.access_time << " ns, access " << gs.access_counter << ", miss " << gs.miss_num;
        cout  << ", replace " << gs.replace_num << ", fetch " << gs.fetch_num << ", prefetch " << gs.prefetch_num << endl;
        cout << "   Miss Rate " << (double)gs.miss_num / (double)gs.access_counter << endl;
        if (outputRate) {
            fout << (double)gs.miss_num / (double)gs.access_counter << " ";
        } else {
            fout << (double)gs.access_time / (double)cnt << " ";
        }
    }
    fout << endl;

    cout << "---------------------------------------------------" << endl;
    cout << "MissRate-Associativity(32 sets and 1024 block size):" << endl;
    lc.size = 1 << 10;
    for (int i = 0; i != 11; ++i) {
        lc.associativity = 1 << i;
        lc.size = 1<<(15+i);
        l1.SetConfig(lc);
        l1.SetStats(s);

        for (int j = 0; j != cnt; ++j) {
            l1.HandleRequest(addr[j], 0, read[j], nullptr, hit, time);
        }
        l1.GetStats(gs);
        cout << "Associativity " << (1<<i) <<endl;
        cout << "   Total access_time " << gs.access_time << " ns, access " << gs.access_counter << ", miss " << gs.miss_num;
        cout  << ", replace " << gs.replace_num << ", fetch " << gs.fetch_num << ", prefetch " << gs.prefetch_num << endl;
        cout << "Miss Rate " << (double)gs.miss_num / (double)gs.access_counter << endl;
        if (outputRate) {
            fout << (double)gs.miss_num / (double)gs.access_counter << " ";
        } else {
            fout << (double)gs.access_time / (double)cnt << " ";
        }
    }
    fout << endl;
    fout.close();
#endif


#else
    Memory m;
    Cache l1, l2;
    l1.SetLower(&l2);
    l2.SetLower(&m);

    StorageStats s;
    s.access_time = 0;
    s.access_counter = 0;
    s.miss_num = 0;
    s.replace_num = 0;
    s.fetch_num = 0;
    s.prefetch_num = 0;
    m.SetStats(s);
    l1.SetStats(s);
    l2.SetStats(s);

    StorageLatency sl;
    sl.bus_latency = 0;
    sl.hit_latency = 30;
    m.SetLatency(sl);

    sl.bus_latency = 0;
    sl.hit_latency = 4;
    l2.SetLatency(sl);

    sl.bus_latency = 0;
    sl.hit_latency = 3;
    l1.SetLatency(sl);

    CacheConfig lc;
    lc.fake = true;
    lc.size = 1<<15;
    lc.associativity = 8;
    lc.setNum = 1<<6;
    lc.writeThrough = 0;
    lc.writeAllocate = 0;
    lc.streamBuffer = true;
    l1.SetConfig(lc);

    lc.size = 1<<18;
    lc.setNum = 1<<9;
    lc.streamBuffer = false;
    l2.SetConfig(lc);

    double totalTime = 0.0;
    int cnt, read;
    uint addr;
    char c;
    cin >> cnt;
    for (int i = 0; i != cnt; ++i) {
        cin >> c >> std::hex >> addr;
        read = c == 'r' ? 1 : 0;
        l1.HandleRequest(addr, 0, read, nullptr, hit, time);
        totalTime += time;
        //l1.Print();
    }

    l1.GetStats(s);
    cout << "L1 total access_time " << s.access_time << " ns, access " << s.access_counter << ", miss " << s.miss_num;
    cout  << ", replace " << s.replace_num << ", fetch " << s.fetch_num << ", prefetch " << s.prefetch_num << endl;
    cout << "       Miss Rate " << (double)s.miss_num / (double)s.access_counter << endl;
    StorageStats ss;
    l1.GetStreamBufferStats(ss);
    cout << "       Streambuffer access " << ss.access_counter << ", miss " << ss.miss_num;
    cout << ", replace " << ss.replace_num << ", fetch " << ss.fetch_num << endl;
    cout << "       Miss Rate " << (double)ss.miss_num / (double)ss.access_counter << endl;
    cout << "Total miss rate " << (double)ss.miss_num / (double)s.access_counter << endl;

    l2.GetStats(s);
    cout << "L2 total access_time " << s.access_time << " ns, access " << s.access_counter << ", miss " << s.miss_num;
    cout  << ", replace " << s.replace_num << ", fetch " << s.fetch_num << ", prefetch " << s.prefetch_num << endl;
    cout << "       Miss Rate " << (double)s.miss_num / (double)s.access_counter << endl;

    cout << "AMAT " << totalTime * 0.5 / (double)cnt << " ns" << endl;


#endif
    return 0;
}
