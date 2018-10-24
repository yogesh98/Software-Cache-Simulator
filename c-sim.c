#include "c-sim.h"

static FILE *traceFile;
static cache Cache;
static statistics Stat;
int count;

int main(int argc, char* argv[])
{
    count=0;
    int assoc, TOTAL_RECOUNT;
    if (!strcmp(argv[2], "direct"))
        assoc = 1;
    else if (!strcmp(argv[2], "assoc"))
        assoc = atoi(argv[1]) / atoi(argv[3]);
    else if (!strncmp(argv[2], "assoc:", 6))
        assoc = atoi(strchr(argv[2], ':')+1);
    traceFile = fopen(argv[6], "r");
    if (traceFile == NULL) {
        printf("error no file exists");
        exit(0);
    }
    generalSetup(atoi(argv[1]), assoc, atoi(argv[3]),argv[4], argv[5]);
    Cache.sets = (set *)malloc(sizeof(set)*Cache.Sets);
    int i;
    for (i=0; i!=Cache.Sets; ++i)
    {
        Cache.sets[i].head = NULL;
        Cache.sets[i].tail = NULL;
        Cache.sets[i].valids = 0;
    }
    Stat.cHits = 0;
    Stat.cMisses = 0;
    Stat.memReads = 0;
    Stat.memWrites = 0;
    char accessType;
    unsigned whoops_who_cares, memeAdd;
    while (fscanf(traceFile, "%x: %c %x\n", &whoops_who_cares, &accessType, &memeAdd) == 3)
    {
        TOTAL_RECOUNT=TOTAL_RECOUNT+1;
        switch(accessType)
        {
            case 'W':
            case 'R':
                if (!strcmp(Cache.rPolicy, "LRU"))
                {
                  accessLRU(memeAdd, accessType);
                  break;
                }
                else
                {
                  accessFIFO(memeAdd, accessType);
                  count=count+1;
                  break;
                }
                break;
            default:
                break;
        }
    }
    printf("Memory reads: %d\n", Stat.memReads);
    printf("Memory writes: %d\n", Stat.memWrites);
    if (!strcmp(Cache.rPolicy, "LRU"))
    {
      printf("Cache hits: %d\n", Stat.cHits);
    }
    else
    {
      printf("Cache hits: %d\n", Stat.cHits);
    }
    printf("Cache misses: %d\n", Stat.cMisses);
    fclose(traceFile);
    return 0;
}

void generalSetup(int Cap, int associativity, int Bsize, char* rPolicy,char* writePolicy)
{
    Cache.Cap = Cap;
    Cache.associativity = associativity;
    Cache.Bsize = Bsize;
    Cache.Sets = Cap / associativity / Bsize;
    Cache.writePolicy = writePolicy;
    Cache.rPolicy=rPolicy;
    Cache.setbits = loglog(Cache.Sets);
    Cache.boffset = loglog(Bsize);
    Cache.tagbits = 32 - Cache.setbits - Cache.boffset;
    Cache.tagMask = 0xFFFFFFFF >> (Cache.setbits + Cache.boffset);
    Cache.tagMaskOffset = Cache.setbits + Cache.boffset;
    Cache.indexMask = 0xFFFFFFFF >> (Cache.tagbits + Cache.boffset);
    if (Cache.Sets == 1)
      Cache.indexMask = 0;
    Cache.indexMaskOffset = Cache.boffset;
}

int loglog(int x)
{
    int y = -1;
    while (x > 0) {
        x >>= 1;
        ++y;
    }
    return y;
}
void accessLRU(unsigned memeAdd, char accessType)
{
    int tag = (memeAdd >> Cache.tagMaskOffset) & Cache.tagMask;
    int index = (memeAdd >> Cache.indexMaskOffset) & Cache.indexMask;
    if (accessType == 'R')
    {
        line* block = (line *)malloc(sizeof(line));
        if (Cache.sets[index].valids == 0)
        {
            Stat.cMisses++;
            Stat.memReads++;
            block->tag = tag;
            block->dirty = 0;
            insertLRU(&Cache.sets[index], block);
        }
        else
        {
            line *itr = Cache.sets[index].head;
            while (itr != NULL) {
                if (itr->tag == tag)
                    break;
                itr = itr->next;
            }
            if (itr == NULL)
            {
                Stat.cMisses++;
                Stat.memReads++;
                if (Cache.sets[index].valids==Cache.associativity)
                {
                    if (!strcmp(Cache.writePolicy, "wb") && Cache.sets[index].tail->dirty)
                        Stat.memWrites++;
                    deleteLRU(&Cache.sets[index], Cache.sets[index].tail);
                }
                block->tag = tag;
                block->dirty = 0;
                insertLRU(&Cache.sets[index], block);
            }
            else
            {
                Stat.cHits++;
                block->tag = itr->tag;
                block->dirty = itr->dirty;
                insertLRU(&Cache.sets[index], block);
                deleteLRU(&Cache.sets[index], itr);
            }
        }
    }
    else {
        if (!strcmp(Cache.writePolicy, "wt"))
            Stat.memWrites++;
        line* block = (line *)malloc(sizeof(line));
        if (Cache.sets[index].valids == 0)
        {
            Stat.cMisses++;
            Stat.memReads++;
            block->tag = tag;
            block->dirty = 1;
            insertLRU(&Cache.sets[index], block);
        }
        else
        {
            line *itr = Cache.sets[index].head;
            while (itr != NULL) {
                if (itr->tag == tag)
                    break;
                itr = itr->next;
            }
            if (itr == NULL) {
                Stat.cMisses++;
                Stat.memReads++;
                if (Cache.sets[index].valids==Cache.associativity)
                {
                    if (!strcmp(Cache.writePolicy, "wb") && Cache.sets[index].tail->dirty)
                        Stat.memWrites++;
                    deleteLRU(&Cache.sets[index], Cache.sets[index].tail);
                }
                block->tag = tag;
                block->dirty = 1;
                insertLRU(&Cache.sets[index], block);
            }
            else {
                Stat.cHits++;
                block->tag = itr->tag;
                block->dirty = 1;
                insertLRU(&Cache.sets[index], block);
                deleteLRU(&Cache.sets[index], itr);
            }
        }
    }
}

void insertLRU(set *s, line* line)
{
    s->valids++;
    line->next = s->head;
    line->prev = NULL;
    if (line->next != NULL) {
        line->next->prev = line;
    }
    else s->tail = line;
    s->head = line;
}

void deleteLRU(set *s, line* line)
{
    s->valids--;
    if (line->prev) {
        line->prev->next = line->next;
    } else {
        s->head = line->next;
    }
    if (line->next) {
        line->next->prev = line->prev;
    } else {
        s->tail = line->prev;
    }
    free(line);
}

void accessFIFO(unsigned memeAdd, char accessType)
{
    int tag = (memeAdd >> Cache.tagMaskOffset) & Cache.tagMask;
    int index = (memeAdd >> Cache.indexMaskOffset) & Cache.indexMask;
    if (accessType == 'R')
    {
        line* block = (line *)malloc(sizeof(line));
        if (Cache.sets[index].valids == 0)
        {
            Stat.cMisses++;
            Stat.memReads++;
            block->tag = tag;
            block->dirty = 0;
            insertHead(&Cache.sets[index], block);
        }
        else
        {
            line *itr = Cache.sets[index].head;
            while (itr != NULL) {
                //printf("%d: %d\n", count, itr->tag);
                if (itr->tag == tag)
                    break;
                itr = itr->next;
            }
            if (itr == NULL) {
              //  printf("%d: %d\n", count, tag);
                Stat.cMisses++;
                Stat.memReads++;
                if (Cache.sets[index].valids==Cache.associativity)
                {
                    if (!strcmp(Cache.writePolicy, "wb") && Cache.sets[index].tail->dirty)
                        Stat.memWrites++;
                    deleteFIFO(&Cache.sets[index], Cache.sets[index].tail);
                }
                block->tag = tag;
                block->dirty = 0;
                insertHead(&Cache.sets[index], block);
            }
            else {
                Stat.cHits++;
                block->tag = itr->tag;
                block->dirty = itr->dirty;
            }
        }
    }

    /* write */
    else
    {
        if (!strcmp(Cache.writePolicy, "wt"))
            Stat.memWrites++;
        line* block = (line *)malloc(sizeof(line));
        if (Cache.sets[index].valids == 0)
        {
            Stat.cMisses++;
            Stat.memReads++;
            block->tag = tag;
            block->dirty = 1;
            insertHead(&Cache.sets[index], block);
        }
        else
        {
            line *itr = Cache.sets[index].head;
            while (itr != NULL) {
                if (itr->tag == tag)
                    break;
                itr = itr->next;
            }
            if (itr == NULL) {
                Stat.cMisses++;
                Stat.memReads++;
                if (Cache.sets[index].valids>=Cache.associativity)
                {
                    if (!strcmp(Cache.writePolicy, "wb") && Cache.sets[index].tail->dirty)
                        Stat.memWrites++;
                    deleteFIFO(&Cache.sets[index], Cache.sets[index].tail);
                }
                block->tag = tag;
                block->dirty = 1;
                insertHead(&Cache.sets[index], block);
            }
            else
            {
                Stat.cHits++;
            }
        }
    }
}

void insertHead(set *s, line* line)
{
    s->valids++;
    line->next = s->head;
    line->prev = NULL;
    if (line->next != NULL) {
        line->next->prev = line;
    }
    else
        s->tail = line;
    s->head = line;
}

void insertInPlace(set *s, line* line)
{
  s->valids++;
}
void deleteFIFO(set *s, line* line)
{
    s->valids--;
    if (line->prev) {
        line->prev->next = line->next;
    } else {
        s->head = line->next;
    }
    if (line->next) {
        line->next->prev = line->prev;
    } else {
        s->tail = line->prev;
    }
    free(line);
}

void freethemALL()
{}
