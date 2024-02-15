//
//  handmade.cpp
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//


void Add2SecSineWave();

void GameInitialize() {
    
    
}

static int frequency = 261;

void GameUpdateAndRender(GameInput *input, GameBackBuffer *backBuffer) {
    static int xOffset = 0;
    static int yOffset = 0;
    
    if(input->controllers[0].left.endedDown) {
        xOffset += 5;
    }
    if(input->controllers[0].right.endedDown) {
        xOffset -= 5;
    }
    if(input->controllers[0].up.endedDown) {
        yOffset += 5;
        frequency += 2;
    }
    if(input->controllers[0].down.endedDown) {
        yOffset -= 5;
        frequency -= 2;
    }

    static bool flag = false;
    if(input->controllers[0].A.endedDown) {
        if(!flag) {
            flag = true;
            //Add2SecSineWave();
        }
    }
    

    
    
    unsigned char *row = (unsigned char *)backBuffer->data;
    for(int y = 0; y < backBuffer->height; ++y) {
        unsigned char *pixel = row;
        for(int x = 0; x < backBuffer->width; ++x) {
            *pixel++ = x + xOffset;
            *pixel++ = y + yOffset;
            *pixel++ = 0;
            *pixel++ = 0xFF;
        }

        row += backBuffer->pitch;
    }
     
    
}
