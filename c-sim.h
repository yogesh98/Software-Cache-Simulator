#ifndef _csim_
#define _csim_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cacheLine {
    unsigned tag;
    int dirty;
    int imp;
    struct cacheLine *next;
    struct cacheLine *prev;
} line;

typedef struct cacheSet {
    int valids;
    line* head;
    line* tail;
}set;

typedef struct cache_ {
    int Cap;
    int associativity;
    int Bsize;
    int Sets;
    int tagbits;
    int setbits;
    int boffset;
    int indexMaskOffset;
    unsigned indexMask;
    int tagMaskOffset;
    unsigned tagMask;
    char* rPolicy;
    char* writePolicy;
    set *sets;
} cache;

typedef struct statistics{
    int memReads;
    int memWrites;
    int cHits;
    int cMisses;
} statistics;

void generalSetup(int, int, int, char*, char*);
int loglog(int x);
void freethemALL();

void accessLRU(unsigned, char);
void insertLRU(set *, line*);
void deleteLRU(set *, line*);

void accessFIFO(unsigned, char);
void deleteFIFO(set *, line*);

void insertHead(set *, line*);


#endif
