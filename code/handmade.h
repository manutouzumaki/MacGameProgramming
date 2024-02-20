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
    const char *(*GetPath)(const char *name, const char *ext);
    GameController controllers[4];
};


typedef int32 SoundHandle;

struct Sound {
    void *data;
    size_t size;
    SoundHandle handle;
};

struct Texture {
    int32 width;
    int32 height;
    uint32 *data;
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


struct Vec2 {
    union {
        struct {
            float32 x;
            float32 y;
        };
        float32 v[2];
    };

    Vec2() : x(0), y(0) {}
    Vec2(float32 x_, float32 y_) : x(x_), y(y_) {}
    float32 operator[](const int32 &index) {
        return v[index];
    }
};

struct AABB {
    Vec2 min;
    Vec2 max; 
};


struct CollisionPacket {
    int32 x;
    int32 y;
    float32 t;
};

struct GameState {

    float32 heroX;
    float32 heroY;

    Arena assetsArena;
    Arena worldArena;

    Sound oliviaRodrigo;
    Sound missionCompleted;

    Texture heroTexture;
    Texture grassTexture;

    uint32 *tiles;
    int32 tilesCountX;
    int32 tilesCountY;

    CollisionPacket frameCollisions[1024];
    int32 frameCollisionCount;

};

#endif
