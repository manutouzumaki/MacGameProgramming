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
    const char *(*GetPath)(const char *name, const char *ext);
    GameController controllers[4];
    float32 deltaTime;
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

struct UV {
    float32 umin;
    float32 vmin;
    float32 umax;
    float32 vmax;
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

struct AABB {
    Vec2 min;
    Vec2 max; 
};

enum TileCollisionType {
    TILE_COLLISION_TYPE_NO_COLLISION,
    TILE_COLLISION_TYPE_16x16,
    TILE_COLLISION_TYPE_8x8_L_U,
    TILE_COLLISION_TYPE_8x8_R_U,
    TILE_COLLISION_TYPE_8x8_L_D,
    TILE_COLLISION_TYPE_8x8_R_D,
    TILE_COLLISION_TYPE_4x4_L_U,
    TILE_COLLISION_TYPE_4x4_R_U,
    TILE_COLLISION_TYPE_4x4_L_D,
    TILE_COLLISION_TYPE_4x4_R_D
};

struct CollisionPacket {
    TileCollisionType type;
    float32 x;
    float32 y;
    float32 sizeX;
    float32 sizeY;
    float32 t;
};

struct CollisionTile {
    CollisionPacket packets[256];
    int32 count;  
};

struct AdjustmentSensor {
    bool mHit;
    bool lHit;
    bool rHit;
};

struct Tilemap {
    uint32 *tiles;
    int32 width;
    int32 height;
};

enum EntityType {
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_ENEMY 
};

struct Entity {
    uint32 uid;
    EntityType type;

    Vec2 pos;
    Vec2 vel; 
    Vec2 dim;

    Vec2 spriteDim;

    Entity *next;
    Entity *prev;
};

struct InputState {
    Vec2 vel;
    float32 inputX;
    float32 inputY;
    float32 deltaTime;
    float64 timeStamp;
};  

struct PacketInput {
    uint32 header;
    uint32 type;
    int32 samplesCount;
    InputState samples[3];
};

struct PacketState {
    uint32 header;
    uint32 type;
    Vec2 pos;
    Vec2 vel;
};

enum ClientState {
    CLIENT_STATE_HELLO,
    CLIENT_STATE_WELCOMED
};

struct GameState {

    Entity *entity;

    Arena networkArena;
    Arena assetsArena;

    Sound oliviaRodrigo;
    Sound missionCompleted;

    Texture heroTexture;
    Texture grassTexture;
    Texture tilemapTexture;

    Tilemap tilemap;
    UV *tilemapUVs;

    // TODO: chagen this to use a Tilemap struct ...
    uint32 *tiles;
    int32 tilesCountX;
    int32 tilesCountY;

    CollisionPacket frameCollisions[1024];
    int32 frameCollisionCount;

    // TODO: change this to use a slotmap or something more cache friendly
    MemoryPool entityPool;
    Entity *entities;
    HashMap<Entity *> networkToEntity;

    float64 totalGameTime;
    InputState inputSamples[3];
    int32  inputSamplesCount;
    float32 lastTimeStamp;


    UDPSocket socket;
    UDPAddress address;

    ClientState clientState;
    UDPAddress sendAddress;

    float32 timePassFromLastInputSampled;
    float32 timePassFromLastInputPacket;
};

#endif

// NOTE: temporal place for Globals
static const float32 MetersToPixels = 32;
static const float32 PixelsToMeters = 1.0f / MetersToPixels;
static const int32 SPRITE_SIZE = 1;
static const uint32 MaxEntityCount = 1024;

static const uint32 PacketHeader      = 'PIPE';
static const uint32 PacketTypeHello   = 'HELO';
static const uint32 PacketTypeState   = 'STAT';
static const uint32 PacketTypeWelcome = 'WLCM';

static const float32 TimeBetweenHellos = 1.f;

static const float32 TimeBetweenInputPackets = 0.033f;
static const float32 TimeBetweenInputSamples = 0.03f;

static const uint32  MaxPacketPerFrameCount = 10;


