//
//  handmade.h
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//
#ifndef handmade_h
#define handmade_h

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
    ENTITY_TYPE_PLAYER
};

struct Entity {
    uint32 uid;
    EntityType type;

    Vec2 pos;
    Vec2 vel; 
    Vec2 dim;

    Vec2 spriteDim;

    UDPAddress address;

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
    uint32 uid;
    int32 samplesCount;
    InputState samples[3];
};


struct Client {
    uint32 uid;
    UDPAddress address;
    Entity *entity;
};

struct GameState {
    Arena assetsArena;
    Arena clientArena;
    Arena packetArena;

    // TODO: chagen this to use a Tilemap struct ...
    uint32 *tiles;
    int32 tilesCountX;
    int32 tilesCountY;

    CollisionPacket frameCollisions[1024];
    int32 frameCollisionCount;

    // TODO: change this to use a slotmap or something more cache friendly
    MemoryPool entityPool;
    Entity *entities;

    UDPSocket socket;
    UDPAddress addrs;

    HashMap<Client> clientsMap;
    uint32 clientCount;

    PacketInput *framePackets;
    int32 framePacketCount;


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

static const float32 TimeBetweenInputPackets = 0.033f;
static const uint32  MaxPacketPerFrameCount = 10;

