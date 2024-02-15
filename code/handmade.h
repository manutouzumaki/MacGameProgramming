//
//  handmade.h
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//
#ifndef handmade_h
#define handmade_h

struct GameButton {
    int halfTransitionCount;
    bool endedDown;
    
};

struct GameController {
    union {
        struct {
            GameButton A;
            GameButton B;
            GameButton X;
            GameButton Y;
            GameButton left;
            GameButton right;
            GameButton up;
            GameButton down;
        };
        GameButton buttons[8];
    };
};

struct GameInput {
    GameController controllers[4];
};

struct GameBackBuffer {
    void *data;
    int width;
    int height;
    int pitch;
};

#endif
