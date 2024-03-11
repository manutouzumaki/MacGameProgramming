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

Arena ArenaCreate(Memory *memory, size_t size);
void *ArenaPushSize(Arena *arena, size_t size);
#define ArenaPushStruct(arena, type) (type *)ArenaPushSize(arena, sizeof(type))
#define ArenaPushArray(arena, count, type) (type *)ArenaPushSize(arena, count * sizeof(type))


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
