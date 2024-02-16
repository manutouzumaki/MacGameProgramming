//
//  handmade.cpp
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//


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

#define ArenaPushStruct(arena, type) (type *)ArenaPushSize(arena, sizeof(type))
#define ArenaPushArray(arena, count, type) (type *)ArenaPushSize(arena, count * sizeof(type))


void GameInitialize(Memory *memory, GameSound *sound) {
    
    ASSERT((memory->used + sizeof(GameState)) <= memory->size);
    
    GameState *gameState = (GameState *)memory->data;
    memory->used += sizeof(GameState);


    gameState->soundArena = ArenaCreate(memory, MB(40));

    gameState->oliviaRodrigo = sound->Load(&gameState->soundArena, "test", true, true);
    gameState->missionCompleted = sound->Load(&gameState->soundArena, "test1", false, false);

}

void GameUpdateAndRender(Memory *memory, GameSound *sound, GameInput *input, GameBackBuffer *backBuffer) {

    GameState *gameState = (GameState *)memory->data;
    
    if(input->controllers[0].left.endedDown) {
        gameState->xOffset += 5;
    }
    if(input->controllers[0].right.endedDown) {
        gameState->xOffset -= 5;
    }
    if(input->controllers[0].up.endedDown) {
        gameState->yOffset += 5;
    }
    if(input->controllers[0].down.endedDown) {
        gameState->yOffset -= 5;
    }

    if(input->controllers[0].A.endedDown) {
        sound->Restart(gameState->missionCompleted);
        sound->Play(gameState->missionCompleted);
    }
     
    uint8 *row = (uint8 *)backBuffer->data;
    for(int32 y = 0; y < backBuffer->height; ++y) {
        uint8 *pixel = row;
        for(int32 x = 0; x < backBuffer->width; ++x) {
            *pixel++ = x + gameState->xOffset;
            *pixel++ = y + gameState->yOffset;
            *pixel++ = 0;
            *pixel++ = 0xFF;
        }

        row += backBuffer->pitch;
    }
     
    
}
