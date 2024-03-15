struct Memory {
    size_t size;
    size_t used;
    uint8 *data;
};

struct Arena {
    size_t size;
    size_t used;
    uint8 *base;
};

struct ArenaTemp {
    Arena *arena;
    size_t pos;  
};

Arena ArenaCreate(Memory *memory, size_t size);
void *ArenaPushSize(Arena *arena, size_t size);
void ArenaClear(Arena *arena);
#define ArenaPushStruct(arena, type) (type *)ArenaPushSize(arena, sizeof(type))
#define ArenaPushArray(arena, count, type) (type *)ArenaPushSize(arena, count * sizeof(type))

ArenaTemp ArenaTempBegin(Arena *arena);
void ArenaTempEnd(ArenaTemp tmp);


struct MemoryPool {
    size_t size;
    size_t elementSize;
    size_t elementCount;
    size_t elementUsed;
    uint8 *data;
    uint32 firstFree;
};

MemoryPool MemoryPoolCreate(Memory *memory, size_t elementCount, size_t elementSize);
void *MemoryPoolAlloc(MemoryPool *pool);
void MemoryPoolRelease(MemoryPool *pool, void *data);

struct MemoryStream {
    uint8 *head;
    uint8 *current;
    size_t size;
};

// TODO: make a read bit stream ...
MemoryStream MemoryStreamCreate();
void MemoryStreamWrite(MemoryStream *stream, void *toWriteBuffer, size_t bufferSize);
void MemoryStreamRead(MemoryStream *stream, void *toReadBuffer, size_t bufferSize);

#define HASH_ELEMENT_DELETED 0xFFFFFFFF

uint32 MurMur2(const void *key, int32 len, uint32 seed);

// TODO: re implement this if is a good solution
template <typename Type>
struct HashMap {
    
    void Initialize(Arena *arena, uint32 size);

    void Add(uint64 key, Type value);
    Type Get(uint64 key);
    Type *GetPtr(uint64 key);
    
    void Add(const char *key, Type value);
    Type *Get(const char *key);

    void Remove(uint64 key);

    void Clear();

    struct HashElement {
        uint32 id;
        Type value;
    };

    uint32 occupied;
    uint32 capacity;
    uint32 mask;
    HashElement *elements;
    
    uint32 seed;
};
