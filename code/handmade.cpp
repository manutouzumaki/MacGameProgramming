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


#ifdef HANDMADE_DEBUG
void DrawDebugRect_(GameBackBuffer *buffer, int32 x, int32 y, int32 width, int32 height, uint32 color) {
    int32 minX = MAX(x, 0);
    int32 minY = MAX(y, 0);
    int32 maxX = MIN(x + width, buffer->width);
    int32 maxY = MIN(y + height, buffer->height);

    uint32 *pixels = (uint32 *)buffer->data;
    for(int32 x = minX; x < maxX; x++) {
        if(minY < maxY) {
            pixels[minY * buffer->width + x] = color; 
            pixels[(maxY - 1) * buffer->width + x] = color; 
        }
    }
    for(int32 y = minY; y < maxY; y++) {
        if(minX < maxX) {
            pixels[y * buffer->width + minX] = color; 
            pixels[y * buffer->width + (maxX - 1)] = color; 
        }
    }
}
#define DrawDebugRect(buffer, x, y, width, height, color) DrawDebugRect_(buffer, x, y, width, height, color)
#else
#define DrawDebugRect(buffer, x, y, width, height, color)
#endif

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

bool AABBVsAABB(AABB a, AABB b) {
    if(a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if(a.max.y < b.min.y || a.min.y > b.max.y) return false;
    return true;
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

// TODO: link to the pass collision adjusment
void CollisionAdjusment(GameBackBuffer *backBuffer, AABB aabbOther, uint32 &color, float32 &centerX, float32 &centerY, float32 inputX, float32 inputY) {
    float32 buttonPressed = inputX * inputX + inputY * inputY;
    if(buttonPressed > 0.0f) {
        
        AABB sensorL;
        AABB sensorM;
        AABB sensorR;

        if(inputX != 0.0f && inputY != 0.0f) {
        }
        else if(inputX > 0.0f) {
        
        }
        else if(inputX < 0.0f) {
            float32 size = 0.9f;
            float32 LRwidth = 0.25f * size;
            float32 Mwidth = 0.4f * size;
            float32 height = 0.125f * size;

            float32 sensorX = centerX - (size*0.5f) - (height*0.5f);
            float32 sensorY = centerY;
            sensorM.min = Vec2(sensorX - height * 0.5f, sensorY - Mwidth * 0.5f);
            sensorM.max = Vec2(sensorX + height * 0.5f, sensorY + Mwidth * 0.5f); 

            sensorX = centerX - (size*0.5f) - (height * 0.5f);
            sensorY = centerY - (size*0.5f) + (LRwidth * 0.5f);
            sensorL.min = Vec2(sensorX - height * 0.5f, sensorY - LRwidth * 0.5f);
            sensorL.max = Vec2(sensorX + height * 0.5f, sensorY + LRwidth * 0.5f);
              
            sensorX = centerX - (size*0.5f) - (height * 0.5f);
            sensorY = centerY + (size*0.5f) - (LRwidth * 0.5f);
            sensorR.min = Vec2(sensorX - height * 0.5f, sensorY - LRwidth * 0.5f);
            sensorR.max = Vec2(sensorX + height * 0.5f, sensorY + LRwidth * 0.5f);

            bool mHit = AABBVsAABB(sensorM, aabbOther);
            bool lHit = AABBVsAABB(sensorL, aabbOther);
            bool rHit = AABBVsAABB(sensorR, aabbOther);

            if(mHit == false) {
                if(lHit) {
                    centerY += 1.0f * 0.075f;
                }
                if(rHit) { 
                    centerY -= 1.0f * 0.075f;
                }
            }

            DrawDebugRect(backBuffer,
                          sensorM.min.x*MetersToPixels,
                          sensorM.min.y*MetersToPixels,
                          (sensorM.max.x-sensorM.min.x)*MetersToPixels,
                          (sensorM.max.y-sensorM.min.y)*MetersToPixels,
                          color);
            DrawDebugRect(backBuffer,
                          sensorL.min.x*MetersToPixels,
                          sensorL.min.y*MetersToPixels,
                          (sensorL.max.x-sensorL.min.x)*MetersToPixels,
                          (sensorL.max.y-sensorL.min.y)*MetersToPixels,
                          0xFFFF00FF);
            DrawDebugRect(backBuffer,
                          sensorR.min.x*MetersToPixels,
                          sensorR.min.y*MetersToPixels,
                          (sensorR.max.x-sensorR.min.x)*MetersToPixels,
                          (sensorR.max.y-sensorR.min.y)*MetersToPixels,
                          0xFFFF00FF);
        }
        else if(inputY > 0.0f) {
        
        }
        else if(inputY < 0.0f) {

            float32 size = 0.9f;
            float32 LRwidth = 0.25f * size;
            float32 Mwidth = 0.4f * size;
            float32 height = 0.125f * size;

            float32 sensorX = centerX;
            float32 sensorY = centerY - (size*0.5f) - (height*0.5f);
            sensorM.min = Vec2(sensorX - Mwidth * 0.5f, sensorY - height * 0.5f);
            sensorM.max = Vec2(sensorX + Mwidth * 0.5f, sensorY + height * 0.5f); 

            sensorX = centerX - (size*0.5f) + (LRwidth * 0.5f);
            sensorY = centerY - (size*0.5f) - (height * 0.5f);
            sensorL.min = Vec2(sensorX - LRwidth * 0.5f, sensorY - height * 0.5f);
            sensorL.max = Vec2(sensorX + LRwidth * 0.5f, sensorY + height * 0.5f);
              
            sensorX = centerX + (size*0.5f) - (LRwidth * 0.5f);
            sensorY = centerY - (size*0.5f) - (height * 0.5f);
            sensorR.min = Vec2(sensorX - LRwidth * 0.5f, sensorY - height * 0.5f);
            sensorR.max = Vec2(sensorX + LRwidth * 0.5f, sensorY + height * 0.5f);

            bool mHit = AABBVsAABB(sensorM, aabbOther);
            bool lHit = AABBVsAABB(sensorL, aabbOther);
            bool rHit = AABBVsAABB(sensorR, aabbOther);

            
            if(mHit == false) {
                if(lHit) {
                    centerX += 1.0f * 0.075f;
                }
                if(rHit) { 
                    centerX -= 1.0f * 0.075f;
                }
            }
            

            DrawDebugRect(backBuffer,
                          sensorM.min.x*MetersToPixels,
                          sensorM.min.y*MetersToPixels,
                          (sensorM.max.x-sensorM.min.x)*MetersToPixels,
                          (sensorM.max.y-sensorM.min.y)*MetersToPixels,
                          color);
            DrawDebugRect(backBuffer,
                          sensorL.min.x*MetersToPixels,
                          sensorL.min.y*MetersToPixels,
                          (sensorL.max.x-sensorL.min.x)*MetersToPixels,
                          (sensorL.max.y-sensorL.min.y)*MetersToPixels,
                          0xFFFF00FF);
            DrawDebugRect(backBuffer,
                          sensorR.min.x*MetersToPixels,
                          sensorR.min.y*MetersToPixels,
                          (sensorR.max.x-sensorR.min.x)*MetersToPixels,
                          (sensorR.max.y-sensorR.min.y)*MetersToPixels,
                          0xFFFF00FF);
        
        }


    }
}

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
        0, 0, 0, 1, 1, 1, 1, TILE_COLLISION_TYPE_4x4_R_D, 0, 0, 1, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 1, 1, TILE_COLLISION_TYPE_4x4_R_D, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        0, 0, 0, 1, 1, TILE_COLLISION_TYPE_4x4_R_D, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 1, TILE_COLLISION_TYPE_4x4_R_D, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, TILE_COLLISION_TYPE_4x4_R_D, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 1, 1, TILE_COLLISION_TYPE_8x8_R_U, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 1, 1, 1, TILE_COLLISION_TYPE_8x8_R_U, 0, 0, 1, 0, 0, 1, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, TILE_COLLISION_TYPE_8x8_R_U, 0, 1, 0, 1, 1, 1, 1, 1,
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
    float32 inputX = 0;
    float32 inputY = 0;
    if(input->controllers[0].left.endedDown) {
        ddpX -= 1;
        inputX -= 1;
    }
    if(input->controllers[0].right.endedDown) {
        ddpX += 1;
        inputX += 1;
    }
    if(input->controllers[0].up.endedDown) {
        ddpY -= 1;
        inputY -= 1;
    }
    if(input->controllers[0].down.endedDown) {
        ddpY += 1;
        inputY += 1;
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
    gameState->frameCollisionCount = 0;
    
    //TODO: olny check the posible tiles, not the entire tilemap
    AABB oldP;
    oldP.min = Vec2(centerX - 0.45f, centerY - 0.45f); 
    oldP.max = Vec2(centerX + 0.45f, centerY + 0.45f); 

    AABB newP;
    newP.min = Vec2(centerX + ddpX - 0.45f, centerY + ddpY - 0.45f); 
    newP.max = Vec2(centerX + ddpX + 0.45f, centerY + ddpY + 0.45f); 

    int32 minX = (int32)floorf(MIN(oldP.min.x, newP.min.x));
    int32 minY = (int32)floorf(MIN(oldP.min.y, newP.min.y));
    int32 maxX = (int32)ceilf(MAX(oldP.max.x, newP.max.x));
    int32 maxY = (int32)ceilf(MAX(oldP.max.y, newP.max.y)); 

    // TODO: clamp to fall inside the tilemap
    minX = MAX(minX, 0);
    minY = MAX(minY, 0);
    maxX = MIN(maxX, gameState->tilesCountX);
    maxY = MIN(maxY, gameState->tilesCountY);
    
    for(int32 y = minY; y < maxY; y++) {
        for(int32 x = minX; x < maxX; x++) {
            
            uint32 tile = gameState->tiles[y * gameState->tilesCountX + x];
            if(tile == TILE_COLLISION_TYPE_16x16) {
                AABB aabbOuter;
                aabbOuter.min = Vec2(x - 0.45f, y - 0.45f);
                aabbOuter.max = Vec2(x + SPRITE_SIZE + 0.45f, y + SPRITE_SIZE + 0.45f); 
               
                Vec2 contactPoint;
                Vec2 contactNorm;
                float32 t = -1.0f; 
                if(RayVsAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabbOuter, contactPoint, contactNorm, t) && t <= 1.0f) {
                    CollisionPacket collision;
                    collision.type = TILE_COLLISION_TYPE_16x16;
                    collision.x = x;
                    collision.y = y;
                    collision.sizeX = SPRITE_SIZE;
                    collision.sizeY = SPRITE_SIZE;
                    collision.t = t;
                    ASSERT((gameState->frameCollisionCount + 1) <= 1024);
                    gameState->frameCollisions[gameState->frameCollisionCount++] = collision;
                }
            }
            if(tile == TILE_COLLISION_TYPE_8x8_R_U) {
                
                float32 posX = x;
                float32 posY = y;
                float32 sizeX = SPRITE_SIZE*0.5f;
                float32 sizeY = SPRITE_SIZE*0.5f;
                 
                for(int32 j = 0; j < 2; j++) {
                    for(int32 i = 0; i < j + 1; i++) {                

                        AABB aabbOuter;
                        aabbOuter.min = Vec2(posX - 0.45f, posY - 0.45f);
                        aabbOuter.max = Vec2(posX + sizeX + 0.45f, posY + sizeY + 0.45f); 
                       
                        Vec2 contactPoint;
                        Vec2 contactNorm;
                        float32 t = -1.0f; 
                        if(RayVsAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabbOuter, contactPoint, contactNorm, t) && t <= 1.0f) {
                            CollisionPacket collision;
                            collision.type = TILE_COLLISION_TYPE_8x8_R_U;
                            collision.x = posX;
                            collision.y = posY;
                            collision.sizeX = sizeX;
                            collision.sizeY = sizeY;
                            collision.t = t;
                            ASSERT((gameState->frameCollisionCount + 1) <= 1024);
                            gameState->frameCollisions[gameState->frameCollisionCount++] = collision;
                        }

                        posX += sizeX;
                    }
                    posX = x;
                    posY += sizeY;
                }

            }
            if(tile == TILE_COLLISION_TYPE_4x4_R_D) {
                float32 posX = x;
                float32 posY = y;
                float32 sizeX = SPRITE_SIZE*0.25f;
                float32 sizeY = SPRITE_SIZE*0.25f;
                
                for(int32 j = 0; j < 4; j++) {
                    for(int32 i = 0; i < 4 - j; i++) {                

                        AABB aabbOuter;
                        aabbOuter.min = Vec2(posX - 0.45f, posY - 0.45f);
                        aabbOuter.max = Vec2(posX + sizeX + 0.45f, posY + sizeY + 0.45f); 
                       
                        Vec2 contactPoint;
                        Vec2 contactNorm;
                        float32 t = -1.0f; 
                        if(RayVsAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabbOuter, contactPoint, contactNorm, t) && t <= 1.0f) {
                            CollisionPacket collision;
                            collision.type = TILE_COLLISION_TYPE_4x4_R_D;
                            collision.x = posX;
                            collision.y = posY;
                            collision.sizeX = sizeX;
                            collision.sizeY = sizeY;
                            collision.t = t;
                            ASSERT((gameState->frameCollisionCount + 1) <= 1024);
                            gameState->frameCollisions[gameState->frameCollisionCount++] = collision;
                        }

                        posX += sizeX;
                    }
                    posX = x;
                    posY += sizeY;
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

        AABB aabb;
        aabb.min = Vec2(collision.x - 0.45f, collision.y - 0.45f);
        aabb.max = Vec2(collision.x + collision.sizeX + 0.45f, collision.y + collision.sizeY + 0.45f);

        Vec2 contactPoint;
        Vec2 contactNorm;
        float32 t = -1.0f; 
        if(RayVsAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabb, contactPoint, contactNorm, t) && t <= 1.0f) {

            centerX = (contactPoint.x + contactNorm.x * 0.0001f);
            centerY = (contactPoint.y + contactNorm.y * 0.0001f);

            float32 tRest = 1.0f - t;
            ddpX *= tRest; 
            ddpY *= tRest;
            ddpX += (contactNorm.x * fabsf(ddpX));
            ddpY += (contactNorm.y * fabsf(ddpY));
        }

    }

    DrawRect(backBuffer, 0, 0, backBuffer->width, backBuffer->height, 0xFFAABBAA);

    uint32 color = 0xFF00FFFF; 
    for(int32 y = minY; y < maxY; y++) {
        for(int32 x = minX; x < maxX; x++) {
            
            uint32 tile = gameState->tiles[y * gameState->tilesCountX + x];
            if(tile == TILE_COLLISION_TYPE_16x16) {
                AABB aabbOther;
                aabbOther.min = Vec2(x, y);
                aabbOther.max = Vec2(x + SPRITE_SIZE, y + SPRITE_SIZE);

                CollisionAdjusment(backBuffer, aabbOther, color, centerX, centerY, inputX, inputY);
               
            }
            if(tile == TILE_COLLISION_TYPE_8x8_R_U) {
                
                float32 posX = x;
                float32 posY = y;
                float32 sizeX = SPRITE_SIZE*0.5f;
                float32 sizeY = SPRITE_SIZE*0.5f;
                 
                for(int32 j = 0; j < 2; j++) {
                    for(int32 i = 0; i < j + 1; i++) {                

                        AABB aabbOther;
                        aabbOther.min = Vec2(posX, posY);
                        aabbOther.max = Vec2(posX + sizeX, posY + sizeY);

                        CollisionAdjusment(backBuffer, aabbOther, color, centerX, centerY, inputX, inputY);

                        posX += sizeX;
                    }
                    posX = x;
                    posY += sizeY;
                }

            }
            if(tile == TILE_COLLISION_TYPE_4x4_R_D) {
                float32 posX = x;
                float32 posY = y;
                float32 sizeX = SPRITE_SIZE*0.25f;
                float32 sizeY = SPRITE_SIZE*0.25f;

                
                for(int32 j = 0; j < 4; j++) {
                    for(int32 i = 0; i < 4 - j; i++) {                

                        AABB aabbOther;
                        aabbOther.min = Vec2(posX, posY);
                        aabbOther.max = Vec2(posX + sizeX, posY + sizeY);

                        CollisionAdjusment(backBuffer, aabbOther, color, centerX, centerY, inputX, inputY);
                       
                        posX += sizeX;
                    }
                    posX = x;
                    posY += sizeY;
                }


            }


        }
    }


    gameState->heroX = centerX - SPRITE_SIZE*0.5f;
    gameState->heroY = centerY - SPRITE_SIZE;

    gameState->heroX += ddpX;
    gameState->heroY += ddpY;

    
 


    // Draw the tilemap
    for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {
            if(gameState->tiles[y * gameState->tilesCountX + x] == 1) {
#if 1
                DrawRectTexture(backBuffer,
                                x * SPRITE_SIZE*MetersToPixels,
                                y * SPRITE_SIZE*MetersToPixels,
                                SPRITE_SIZE*MetersToPixels, SPRITE_SIZE*MetersToPixels,
                                gameState->grassTexture);
#endif

                DrawDebugRect(backBuffer,
                              x * SPRITE_SIZE*MetersToPixels,
                              y * SPRITE_SIZE*MetersToPixels,
                              SPRITE_SIZE*MetersToPixels, SPRITE_SIZE*MetersToPixels,
                              0xFF00FF00);
            }
            if(gameState->tiles[y * gameState->tilesCountX + x] == TILE_COLLISION_TYPE_8x8_R_U) {
                float32 posX = x;
                float32 posY = y;
                float32 sizeX = SPRITE_SIZE*0.5f;
                float32 sizeY = SPRITE_SIZE*0.5f;
                 
                for(int32 j = 0; j < 2; j++) {
                    for(int32 i = 0; i < j + 1; i++) {                
                        DrawDebugRect(backBuffer,
                                      posX * SPRITE_SIZE*MetersToPixels,
                                      posY * SPRITE_SIZE*MetersToPixels,
                                      sizeX*MetersToPixels, sizeY*MetersToPixels,
                                      0xFF00FF00);
                        posX += sizeX;
                    }
                    posX = x;
                    posY += sizeY;
                }
            }
            if(gameState->tiles[y * gameState->tilesCountX + x] == TILE_COLLISION_TYPE_4x4_R_D) {
                float32 posX = x;
                float32 posY = y;
                float32 sizeX = SPRITE_SIZE*0.25f;
                float32 sizeY = SPRITE_SIZE*0.25f;
                
                for(int32 j = 0; j < 4; j++) {
                    for(int32 i = 0; i < 4 - j; i++) {                
                        DrawDebugRect(backBuffer,
                                      posX * SPRITE_SIZE*MetersToPixels,
                                      posY * SPRITE_SIZE*MetersToPixels,
                                      sizeX*MetersToPixels, sizeY*MetersToPixels,
                                      0xFF00FF00);
                        posX += sizeX;
                    }
                    posX = x;
                    posY += sizeY;
                }

            }
        }
    }

    centerX = gameState->heroX + SPRITE_SIZE*0.5f;
    centerY = gameState->heroY + SPRITE_SIZE;

#if 1 
    DrawRectTexture(backBuffer, 
                    gameState->heroX*MetersToPixels,
                    gameState->heroY*MetersToPixels,
                    SPRITE_SIZE*MetersToPixels,
                    SPRITE_SIZE*1.5*MetersToPixels,
                    gameState->heroTexture);  
#endif

    DrawDebugRect(backBuffer,
                  (centerX - 0.45f)*MetersToPixels,
                  (centerY - 0.45f)*MetersToPixels,
                  0.9f*MetersToPixels, 0.9f*MetersToPixels,
                  0xFFFFFF00);
   
}
