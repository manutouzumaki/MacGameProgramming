//
//  handmade.h
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//
#ifndef handmade_h
#define handmade_h

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


typedef int32 SoundHandle;

struct Sound {
    void *data;
    size_t size;
    SoundHandle handle;
};

struct GameSound {
    // TODO: the Load function not necesary have to add a sound to a channel
    Sound (*Load) (Arena *arena, const char *name, bool playing, bool loop);
    void (*Remove) (Sound *sound);
    void (*Play) (Sound sound);
    void (*Pause) (Sound sound);
    void (*Restart) (Sound sound);
};

struct GameBackBuffer {
    void *data;
    int width;
    int height;
    int pitch;
};

struct GameState {
    int32 xOffset;
    int32 yOffset;

    Arena soundArena;

    Sound oliviaRodrigo;
    Sound missionCompleted;
};

#endif
