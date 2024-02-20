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

Vec2 ClosestPtPointAABB(Vec2 p, AABB b) {
    Vec2 q = Vec2();
    for(int32 i = 0; i < 2; ++i) {
        float32 v = p[i];
        if(v < b.min[i]) v = b.min[i];
        if(v > b.max[i]) v = b.max[i];
        q.v[i] = v;
    }
    return q;
}

int32 IntersectRayAABB(Vec2 p, Vec2 d, AABB a, float32 &tmin) {
    tmin = 0.0f;// set to -FLT_MAX to get first hit on line
    float32 tmax = FLT_MAX;

    for(int32 i = 0; i < 2; i++) {
        if(fabsf(d[i]) < FLT_EPSILON) {
            // ray is parallel to the slab. no hit is the origin is not inside
            if(p[i] < a.min[i] || p[i] > a.max[i]) return 0;
        }
        else {
            // Compute intersection t of a ray with near and far planof slab
            float32 ood = 1.0f / d[i];
            float32 t1 = (a.min[i] - p[i]) * ood;
            float32 t2 = (a.max[i] - p[i]) * ood;
            if(t1 > t2) {
                float32 tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            tmin = MAX(tmin, t1);
            tmax = MIN(tmax, t2);

            if(tmin > tmax) return 0;
        }
    }
    return 1;
}


static const float32 MetersToPixels = 16*4;
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
    for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {

            if(gameState->tiles[y * gameState->tilesCountX + x] == 1) {

                AABB aabb;
                aabb.min = Vec2(x, y);
                aabb.max = Vec2(x + SPRITE_SIZE, y + SPRITE_SIZE); 

                AABB aabbOuter;
                aabbOuter.min = Vec2(x - 0.25f, y - 0.25f);
                aabbOuter.max = Vec2(x + SPRITE_SIZE + 0.25f, y + SPRITE_SIZE + 0.25f); 
                
                float32 t = -1.0f;

                float32 len = sqrtf(ddpX * ddpX + ddpY * ddpY);
                if(IntersectRayAABB(Vec2(centerX, centerY), Vec2(ddpX, ddpY), aabbOuter, t) && t < len) {
                    Vec2 q = Vec2(
                        centerX + ddpX * t,
                        centerY + ddpY * t
                    );

                    Vec2 closest = ClosestPtPointAABB(q, aabb);
                    float32 diffX = q.x - closest.x;
                    float32 diffY = q.y - closest.y;
                    float32 invLen = 1.0f / sqrtf(diffX * diffX + diffY * diffY);       
                    
                    Vec2 normal = Vec2(
                        diffX * invLen,      
                        diffY * invLen      
                    );

                    centerX = (q.x + normal.x * 0.001f);
                    centerY = (q.y + normal.y * 0.001f);

                    float32 tRest = 1.0f - t;
                    ddpX *= tRest; 
                    ddpY *= tRest;
                    ddpX += (normal.x * fabsf(ddpX));
                    ddpY += (normal.y * fabsf(ddpY));
                }
            }

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
