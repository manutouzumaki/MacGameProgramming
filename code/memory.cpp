// Arena implementation

Arena ArenaCreate(Memory *memory, size_t size) {
    ASSERT((memory->used + size) <= memory->size);

    Arena arena;
    arena.used = 0;
    arena.size = size;
    arena.base = memory->data + memory->used;

    memory->used += size;

    return arena;
}

void *ArenaPushSize(Arena *arena, size_t size) {
    ASSERT((arena->used + size) <= arena->size);

    void *data = arena->base + arena->used;

    arena->used += size;

    return data;
}

// MemoryPool implementation

MemoryPool MemoryPoolCreate(Memory *memory, size_t elementCount, size_t elementSize) {
    size_t size = elementCount * elementSize;
    ASSERT((memory->used + size) <= memory->size);

    MemoryPool pool;
    pool.size = size;
    pool.elementSize = elementSize;
    pool.elementCount = elementCount;
    pool.elementUsed = 0;
    pool.data = memory->data + memory->used;
    pool.firstFree = 0;

    memory->used += size;

    // initilize the free list ...
    uint8 *iterator = (uint8 *)pool.data;
    for(size_t i = 0; i < elementCount; ++i) {
        *((uint32 *)iterator) = i + 1;
        iterator += elementSize;
    }

    return pool;
}

void *MemoryPoolAlloc(MemoryPool *pool) {
    ASSERT(pool->elementUsed + 1 <= pool->elementCount);
    size_t offset = pool->elementSize * pool->firstFree; 
    void *data = pool->data + offset;
    pool->firstFree = *((uint32 *)data); 
    pool->elementUsed++;
    return data;
}

void MemoryPoolRelease(MemoryPool *pool, void *data) {
    ASSERT(pool->elementUsed - 1 >= 0);
    size_t offsetInByte = (size_t)data - (size_t)pool->data;
    uint32 index = offsetInByte / pool->elementSize;
    *((uint32 *)data) = pool->firstFree;
    pool->firstFree = index;
    pool->elementUsed--;
}
