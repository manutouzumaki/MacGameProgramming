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

void ArenaClear(Arena *arena) {
    arena->used = 0;
}

ArenaTemp ArenaTempBegin(Arena *arena) {
    ArenaTemp temp = {0};
    temp.arena = arena;
    temp.pos = arena->used;
    return temp;
}

void ArenaTempEnd(ArenaTemp temp) {
    temp.arena->used = temp.pos;
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


MemoryStream MemoryStreamCreate(void *buffer, size_t bufferSize) {
    MemoryStream stream;
    stream.head = (uint8 *)buffer;
    stream.current = stream.head;
    stream.size = bufferSize;
    return stream;
}

void MemoryStreamWrite(MemoryStream *stream, void *dataToWrite, size_t dataSize) {
    ASSERT(((stream->current + dataSize) - stream->head) < stream->size);
    memcpy(stream->current, dataToWrite, dataSize);
    stream->current += dataSize;
}

void MemoryStreamRead(MemoryStream *stream, void *outData, size_t dataSize) {
    ASSERT(((stream->current + dataSize) - stream->head) < stream->size);
    memcpy(outData, stream->current, dataSize);
    stream->current += dataSize;
}

void MemoryStreamReset(MemoryStream *stream) {
    stream->current = stream->head;
}

// HashMap -----------------------------------------------------------------------------------------
uint32 MurMur2(const void *key, int32 len, uint32 seed) {
    const uint32 m = 0x5bd1e995;
    const int32 r = 24;
    uint32 h = seed ^ len;
    const uint8 *data = (const uint8 *)key;
    while(len >= 4) {
        uint32 k = *(uint32 *)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

template <typename Type>
void HashMap<Type>::Initialize(Arena *arena, uint32 size) {
    ASSERT(IS_POWER_OF_TWO(size));
    capacity = size;
    mask     = (size - 1);
    occupied = 0;

    elements = (HashElement *)ArenaPushSize(arena, sizeof(HashElement) * capacity);
    memset(elements, 0, sizeof(HashElement) * capacity);
    seed = 123;
}


template <typename Type>
void HashMap<Type>::Clear() {
    occupied = 0;
    memset(elements, 0, sizeof(HashElement) * capacity);
}


template <typename Type>
void HashMap<Type>::Add(uint64 key, Type value) {

    ASSERT(occupied + 1 <= capacity);

    uint32 id = MurMur2(&key, sizeof(uint64), seed);
    uint32 index = (id & mask);

    ASSERT(id != HASH_ELEMENT_DELETED);

    if(elements[index].id == 0 || elements[index].id == HASH_ELEMENT_DELETED) {
        HashElement *element = elements + index;
        element->id = id;
        element->value = value;
        occupied++;
    } else {
        uint32 nextIndex = (index + 1) % capacity;
        while(elements[nextIndex].id != 0 && elements[nextIndex].id != HASH_ELEMENT_DELETED) {
            nextIndex = (nextIndex + 1) % capacity;
        }
        HashElement *element = elements + nextIndex;
        element->id = id;
        element->value = value;
        occupied++;
    }
}

template <typename Type>
void HashMap<Type>::Add(const char *key, Type value) {

    ASSERT(occupied + 1 <= capacity);

    uint32 id = MurMur2(key, strlen(key), seed);
    uint32 index = (id & mask);
    
    ASSERT(id != HASH_ELEMENT_DELETED);

    if(elements[index].id == 0 || elements[index].id == HASH_ELEMENT_DELETED) {
        HashElement *element = elements + index;
        element->id = id;
        element->value = value;
        occupied++;
    } else {
        uint32 nextIndex = (index + 1) % capacity;
        while(elements[nextIndex].id != 0 && elements[nextIndex].id != HASH_ELEMENT_DELETED) {
            nextIndex = (nextIndex + 1) % capacity;
        }
        HashElement *element = elements + nextIndex;
        element->id = id;
        element->value = value;
        occupied++;
    }
}

template<typename Type>
void HashMap<Type>::Remove(uint64 key) {

    uint32 id = MurMur2(&key, sizeof(uint64), seed);
    uint32 index = (id & mask);

    ASSERT(id != HASH_ELEMENT_DELETED);

    uint32 counter = 0;
    while((elements[index].id != 0 && elements[index].id != id) && counter < capacity) {
        index = (index + 1) % capacity;
        ++counter;
    }

    if(elements[index].id != 0 && counter <= capacity) {
        elements[index].id = HASH_ELEMENT_DELETED;
        occupied--;
    }
    else {
        printf("Element you are trying to delete was not found\n");
    }
}

template <typename Type>
Type HashMap<Type>::Get(uint64 key) {

    uint32 id = MurMur2(&key, sizeof(uint64), seed);
    uint32 index = (id & mask);

    ASSERT(id != HASH_ELEMENT_DELETED);

    uint32 counter = 0;
    while((elements[index].id != 0 && elements[index].id != id) && counter < capacity) {
        index = (index + 1) % capacity;
        ++counter;
    }

    if(elements[index].id != 0 && elements[index].id != HASH_ELEMENT_DELETED && counter <= capacity) {
        return elements[index].value;
    }
    
    Type zero = {0};
    return zero;
}

template <typename Type>
Type *HashMap<Type>::GetPtr(uint64 key) {
    uint32 id = MurMur2(&key, sizeof(uint64), seed);
    uint32 index = (id & mask);
    uint32 counter = 0;

    ASSERT(id != HASH_ELEMENT_DELETED);
    
    while((elements[index].id != 0 && elements[index].id != id) && counter < capacity) {
        index = (index + 1) % capacity;
        ++counter;
    }

    if(elements[index].id != 0 && elements[index].id != HASH_ELEMENT_DELETED && counter <= capacity) {
        return &elements[index].value;
    }

    return nullptr;    
}

template <typename Type>
Type *HashMap<Type>::Get(const char *key) {
    uint32 id = MurMur2(key, strlen(key), seed);
    uint32 index = (id & mask);
    uint32 counter = 0;

    ASSERT(id != HASH_ELEMENT_DELETED);
    
    while((elements[index].id != 0 && elements[index].id != id) && counter < capacity) {
        index = (index + 1) % capacity;
        ++counter;
    }

    if(elements[index].id != 0 && elements[index].id != HASH_ELEMENT_DELETED && counter <= capacity) {
        return &elements[index].value;
    }

    return nullptr;    
}
