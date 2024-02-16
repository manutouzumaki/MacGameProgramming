//
//  handmade.cpp
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//

static MacSoundHandle testSound1Handle = -1;

void GameInitialize(Memory memory, GameSound *sound) {
    // TODO: ASSERT((memory.used + sizeof(GameState)) <= memory.size);
    GameState *gameState = (GameState *)memory.data;
    memory.used += sizeof(GameState);

    gameState->oliviaRodrigo = sound->Load("test", true, true);
    gameState->missionCompleted = sound->Load("test1", false, false);

}

void GameUpdateAndRender(Memory memory, GameSound *sound, GameInput *input, GameBackBuffer *backBuffer) {

    GameState *gameState = (GameState *)memory.data;
    
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
