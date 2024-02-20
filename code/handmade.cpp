//
//  handmade.cpp
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


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

Texture LoadTexture(Arena *arena, char const *path) {
    Texture texture = {};
    int32 w, h, comp;
    void *data = stbi_load(path, &w, &h, &comp, 4);
    if(data) {
        texture.width = w;
        texture.height = h;
        texture.data = (uint32 *)ArenaPushSize(arena, w * h * comp);
        uint32 *pixels = (uint32 *)data;
        for(uint32 i = 0; i < w * h * comp; i++) {
            int32 A = (pixels[i] >> 24) & 0xFF;
            int32 B = (pixels[i] >> 16) & 0xFF;
            int32 G = (pixels[i] >> 8 ) & 0xFF;
            int32 R = (pixels[i] >> 0 ) & 0xFF;
            texture.data[i] = (A << 24) | (R << 16) | (G << 8) | (B << 0);
        }
        stbi_image_free(data); 
    }
    return texture;
}



void DrawRect(GameBackBuffer *buffer, int32 x, int32 y, int32 width, int32 height, uint32 color) {

    int32 minX = MAX(x, 0);
    int32 minY = MAX(y, 0);
    int32 maxX = MIN(x + width, buffer->width);
    int32 maxY = MIN(y + height, buffer->height);

    uint32 *pixels = (uint32 *)buffer->data;
    for(int32 y = minY; y < maxY; y++) {
        for(int32 x = minX; x < maxX; x++) {
            pixels[y * buffer->width + x] = color; 
        }
    }

}

// TODO: map the textur to the rect
void DrawRectTexture(GameBackBuffer *buffer, int32 x, int32 y, int32 width, int32 height, Texture texture) {

    int32 minX = MAX(x, 0);
    int32 minY = MAX(y, 0);
    int32 maxX = MIN(x + width, buffer->width);
    int32 maxY = MIN(y + height, buffer->height);

    int32 offsetX = -MIN(x, 0);
    int32 offsetY = -MIN(y, 0);
    
    uint32 *pixels = (uint32 *)buffer->data;
    for(int32 y = minY; y < maxY; y++) {

        
        int32 texY = texture.height * ((float32)(y - minY + offsetY) / (float32)height);
        for(int32 x = minX; x < maxX; x++) {
            int32 texX = texture.width * ((float32)(x - minX + offsetX) / (float32)width);
        
            uint32 src = texture.data[texY * texture.width + texX];
            uint32 dst = pixels[y * buffer->width + x];
            
            float32 A = (float32)((src >> 24) & 0x000000FF) / 255.0f;

            int32 srcR = (src >> 16) & 0xFF;
            int32 srcG = (src >> 8 ) & 0xFF;
            int32 srcB = (src >> 0 ) & 0xFF;

            int32 dstA = (dst >> 16) & 0xFF;
            int32 dstR = (dst >> 16) & 0xFF;
            int32 dstG = (dst >> 8 ) & 0xFF;
            int32 dstB = (dst >> 0 ) & 0xFF;

            int32 colR = (1 - A) * dstR + A * srcR;
            int32 colG = (1 - A) * dstG + A * srcG;
            int32 colB = (1 - A) * dstB + A * srcB;

            int32 color = (dstA << 24) | (colR << 16) | (colG << 8) | (colB << 0);
            
            pixels[y * buffer->width + x] = color;
        }
    }

}

void Swap(float32 &a, float32 &b) {
    float32 tmp = a;
    a = b;
    b = tmp;
}

void Swap(int32 &a, int32 &b) {
    int32 tmp = a;
    a = b;
    b = tmp;
}


bool RayVsAABB(Vec2 origin, Vec2 dir, AABB rect, Vec2 &contactPoint, Vec2 &contactNorm, float32 &tHitNear) {

    float32 tNearX = (rect.min.x - origin.x) / dir.x;
    float32 tFarX  = (rect.max.x - origin.x) / dir.x; 

    float32 tNearY = (rect.min.y - origin.y) / dir.y;
    float32 tFarY  = (rect.max.y - origin.y) / dir.y;

    if(isnan(tFarY) || isnan(tFarX)) return false;
    if(isnan(tNearY) || isnan(tNearX)) return false;

    if(tNearX > tFarX) Swap(tNearX, tFarX);
    if(tNearY > tFarY) Swap(tNearY, tFarY);

    if(tNearX > tFarY || tNearY > tFarX) return false;

    tHitNear = MAX(tNearX, tNearY);
    float32 tHitMax = MIN(tFarX, tFarY);
    if(tHitMax < 0) return false;

    contactPoint.x = origin.x + tHitNear * dir.x;
    contactPoint.y = origin.y + tHitNear * dir.y;

    if(tNearX > tNearY) {
        if(dir.x < 0) 
            contactNorm = Vec2(1, 0);
        else
            contactNorm = Vec2(-1, 0);
    }
    else if(tNearX < tNearY) {
        if(dir.y < 0)
            contactNorm = Vec2(0, 1);
        else
            contactNorm = Vec2(0, -1);
    }

    return true;

}


static const float32 MetersToPixels = 32;
static const float32 PixelsToMeters = 1.0f / MetersToPixels;
static const int32 SPRITE_SIZE = 1;


void GameInitialize(Memory *memory, GameSound *sound, GameInput *input) {
    
    ASSERT((memory->used + sizeof(GameState)) <= memory->size);
    
    GameState *gameState = (GameState *)memory->data;
    memory->used += sizeof(GameState);


    gameState->worldArena = ArenaCreate(memory, MB(2));
    gameState->assetsArena = ArenaCreate(memory, MB(40));

    gameState->oliviaRodrigo = sound->Load(&gameState->assetsArena, "test", false, true);
    gameState->missionCompleted = sound->Load(&gameState->assetsArena, "test1", false, false);

    const char *heroPath = input->GetPath("link", "png"); 
    gameState->heroTexture = LoadTexture(&gameState->assetsArena, heroPath);

    const char *grassPath = input->GetPath("grass", "png"); 
    gameState->grassTexture = LoadTexture(&gameState->assetsArena, grassPath);


    // TODO: test tilemap 
    uint32 tempTiles[16*16] = {
        1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    gameState->tilesCountX = 16;
    gameState->tilesCountY = 16;
    gameState->tiles = (uint32 *)ArenaPushArray(&gameState->worldArena, gameState->tilesCountX * gameState->tilesCountY, uint32);
    memcpy(gameState->tiles, tempTiles, gameState->tilesCountX * gameState->tilesCountY * sizeof(uint32));

    // hero start position
    gameState->heroX = 1;
    gameState->heroY = 1;


}

void GameUpdateAndRender(Memory *memory, GameSound *sound, GameInput *input, GameBackBuffer *backBuffer) {

    GameState *gameState = (GameState *)memory->data;

    if(input->controllers[0].A.endedDown) {
        sound->Restart(gameState->missionCompleted);
        sound->Play(gameState->missionCompleted);
    }
     
    float32 ddpX = 0;
    float32 ddpY = 0;
    if(input->controllers[0].left.endedDown) {
        ddpX -= 1;
    }
    if(input->controllers[0].right.endedDown) {
        ddpX += 1;
    }
    if(input->controllers[0].up.endedDown) {
        ddpY -= 1;
    }
    if(input->controllers[0].down.endedDown) {
        ddpY += 1;
    }

    float32 lenSq = ddpX * ddpX + ddpY * ddpY;
    if(lenSq > 1.0f) {
        float32 invLen = 1.0f/sqrtf(lenSq);
        ddpX *= invLen;
        ddpY *= invLen;
    }

    ddpX *= 0.075f;
    ddpY *= 0.075f;

    float32 centerX = gameState->heroX + SPRITE_SIZE*0.5f;
    float32 centerY = gameState->heroY + SPRITE_SIZE;

    // check simple collisions
    //TODO: olny check the posible tiles, not the entire tilemap
    gameState->frameCollisionCount = 0;
    for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {
            
            int32 tileIndex = y * gameState->tilesCountX + x;
            if(gameState->tiles[tileIndex] == 1) {
                AABB aabbOuter;
                aabbOuter.min = Vec2(x - 0.45f, y - 0.45f);
                aabbOuter.max = Vec2(x + SPRITE_SIZE + 0.45f, y + SPRITE_SIZE + 0.45f); 
               
                Vec2 contactPoint;
                Vec2 contactNorm;
                float32 t = -1.0f; 
                if(RayVsAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabbOuter, contactPoint, contactNorm, t) && t <= 1.0f) {
                    CollisionPacket collision;
                    collision.x = x;
                    collision.y = y;
                    collision.t = t;
                    ASSERT((gameState->frameCollisionCount + 1) <= 1024);
                    gameState->frameCollisions[gameState->frameCollisionCount++] = collision;
                }
            }

        }
    }

    // sort the collision base in their time t
    {
        int32 i, j;
        bool swapped;
        for (i = 0; i < gameState->frameCollisionCount - 1; i++) {
            swapped = false;
            for (j = 0; j < gameState->frameCollisionCount - i - 1; j++) {
                if (gameState->frameCollisions[j].t > gameState->frameCollisions[j + 1].t) {

                    CollisionPacket tmp = gameState->frameCollisions[j];
                    gameState->frameCollisions[j] = gameState->frameCollisions[j + 1];
                    gameState->frameCollisions[j + 1] = tmp;

                    swapped = true;
                }
            }
     
            // If no two elements were swapped by inner loop,
            // then break
            if (swapped == false)
                break;
        }
    }
    

    for(int32 i = 0; i < gameState->frameCollisionCount; i++) {

        CollisionPacket collision = gameState->frameCollisions[i];

        AABB aabbOuter;
        aabbOuter.min = Vec2(collision.x - 0.45f, collision.y - 0.45f);
        aabbOuter.max = Vec2(collision.x + SPRITE_SIZE + 0.45f, collision.y + SPRITE_SIZE + 0.45f);

        Vec2 contactPoint;
        Vec2 contactNorm;
        float32 t = -1.0f; 
        if(RayVsAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabbOuter, contactPoint, contactNorm, t) && t <= 1.0f) {

            centerX = (contactPoint.x + contactNorm.x * 0.0001f);
            centerY = (contactPoint.y + contactNorm.y * 0.0001f);

            float32 tRest = 1.0f - t;
            ddpX *= tRest; 
            ddpY *= tRest;
            ddpX += (contactNorm.x * fabsf(ddpX));
            ddpY += (contactNorm.y * fabsf(ddpY));
        }

    }

    gameState->heroX = centerX - SPRITE_SIZE*0.5f;
    gameState->heroY = centerY - SPRITE_SIZE;

    gameState->heroX += ddpX;
    gameState->heroY += ddpY;
 
    DrawRect(backBuffer, 0, 0, backBuffer->width, backBuffer->height, 0xFFAABBAA);


    // Draw the tilemap
    for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {
            if(gameState->tiles[y * gameState->tilesCountX + x] == 1) {
                DrawRectTexture(backBuffer,
                                x * SPRITE_SIZE*MetersToPixels,
                                y * SPRITE_SIZE*MetersToPixels,
                                SPRITE_SIZE*MetersToPixels, SPRITE_SIZE*MetersToPixels,
                                gameState->grassTexture);
            }
        }
    }

    DrawRectTexture(backBuffer, 
                    gameState->heroX*MetersToPixels,
                    gameState->heroY*MetersToPixels,
                    SPRITE_SIZE*MetersToPixels,
                    SPRITE_SIZE*1.5*MetersToPixels,
                    gameState->heroTexture);    
}
