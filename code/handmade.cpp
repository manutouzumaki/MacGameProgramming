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
    NSLog(@"before: %s\n", path);
    void *data = stbi_load(path, &w, &h, &comp, 4);
    if(data) {
        texture.width = w;
        texture.height = h;
        texture.data = (uint32 *)ArenaPushSize(arena, w * h * comp);
        uint32 *pixels = (uint32 *)data;
        for(uint32 i = 0; i < w * h; i++) {
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

Tilemap LoadCSVTilemap(Arena *arena, const char *szFileName, int32 width, int32 height, bool collision = false) {

    FILE *file = fopen(szFileName, "r");
    if(!file) {
        // TODO: handle this
    }
    // go to the end of the file
    fseek(file, 0, SEEK_END);
    // get the size of the file to alloc the memory we need
    long int fileSize = ftell(file);
    // go back to the start of the file
    fseek(file, 0, SEEK_SET);
    // alloc the memory
    uint8 *fileData = (uint8 *)ArenaPushSize(arena, fileSize + 1);
    memset(fileData, 0, fileSize + 1);
    // store the content of the file
    fread(fileData, fileSize, 1, file);
    fileData[fileSize] = 0; // null terminating string...
    fclose(file);


    Tilemap result; 
    result.tiles = (uint32 *)ArenaPushSize(arena, width * height * sizeof(uint32));
    result.width = width;
    result.height = height;
    
    int32 counter = 0;
    char *value = (char *)fileData;
    while(*value) {
        int size = 0;
        
        char *iterator = value;
        while(*iterator != ',' && *iterator != '\n' && *iterator != 0) {
            size++;
            iterator++;
        }

        static char buffer[32];
        memcpy(buffer, value, size);
        buffer[size] = '\0';
        int32 tile = atoi(buffer);
        result.tiles[counter++] = collision ? (uint32)(tile + 1) : (uint32)tile;
        
        value += size + 1;
    }

    return result;
}

UV *GenerateUVs(Arena *arena, int32 tileWidth, int32 tileHeight, Texture texture) {

    int32 countX = texture.width / tileWidth;
    int32 countY = texture.height / tileHeight;
    
    UV *uvs = (UV *)ArenaPushArray(arena, countX * countY, UV);

    float32 uSize = (float32)tileWidth / (float32)texture.width;
    float32 vSize = (float32)tileHeight / (float32)texture.height;
    
    float32 vmin = 0;
    for(int32 y = 0; y < countY; ++y) {
        float32 umin = 0;
        for(int32 x = 0; x < countX; ++x) {
            UV uv;
            uv.umin = umin;
            uv.vmin = vmin;
            uv.umax = umin + uSize;
            uv.vmax = vmin + vSize; 
            uvs[y * countX + x] = uv;

            umin += uSize;        
        }
        vmin += vSize;
    }

    return uvs;
    

}

void GameInitialize(Memory *memory, GameSound *sound, GameInput *input) {
    
    ASSERT((memory->used + sizeof(GameState)) <= memory->size);
    
    GameState *gameState = (GameState *)memory->data;
    memory->used += sizeof(GameState);


    gameState->assetsArena = ArenaCreate(memory, MB(500));

    gameState->oliviaRodrigo = sound->Load(&gameState->assetsArena, "test", true, true);
    gameState->missionCompleted = sound->Load(&gameState->assetsArena, "test1", false, false);

    const char *heroPath = input->GetPath("link", "png"); 
    gameState->heroTexture = LoadTexture(&gameState->assetsArena, heroPath);

    const char *grassPath = input->GetPath("grass", "png"); 
    gameState->grassTexture = LoadTexture(&gameState->assetsArena, grassPath);

    const char *tilemapTexturePath = input->GetPath("tilemap", "png");
    gameState->tilemapTexture = LoadTexture(&gameState->assetsArena, tilemapTexturePath);

    const char *tilemapPath = input->GetPath("tilemap", "csv");
    gameState->tilemap = LoadCSVTilemap(&gameState->assetsArena, tilemapPath, 16, 16);

    const char *collisionPath = input->GetPath("collision", "csv");
    Tilemap collision = LoadCSVTilemap(&gameState->assetsArena, collisionPath, 16, 16, true);

    gameState->tilemapUVs = GenerateUVs(&gameState->assetsArena, 16, 16, gameState->tilemapTexture);

    gameState->tilesCountX = collision.width;
    gameState->tilesCountY = collision.height;
    gameState->tiles = collision.tiles;

    // hero start position
    gameState->heroX = 0;
    gameState->heroY = 0;


}

void GameUpdateAndRender(Memory *memory, GameSound *sound, GameInput *input, GameBackBuffer *backBuffer) {

    GameState *gameState = (GameState *)memory->data;

    float32 dt = input->deltaTime;

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

    ddpX *= 0.075f * 60.0f * dt;
    ddpY *= 0.075f * 60.0f * dt;

    float32 centerX = gameState->heroX + SPRITE_SIZE*0.5f;
    float32 centerY = gameState->heroY + SPRITE_SIZE;

    // check simple collisions
    gameState->frameCollisionCount = 0;
    
    // olny check the posible tiles, not the entire tilemap
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

    // clamp to fall inside the tilemap
    minX = MAX(minX, 0);
    minY = MAX(minY, 0);
    maxX = MIN(maxX, gameState->tilesCountX);
    maxY = MIN(maxY, gameState->tilesCountY);
    
    // Generate Frame Collision Data And Collision Detection
    for(int32 y = minY; y < maxY; y++) {
        for(int32 x = minX; x < maxX; x++) {
            
            TileCollisionType tileType = (TileCollisionType)gameState->tiles[y * gameState->tilesCountX + x];
            CollisionTile collision = GenerateCollisionTile(tileType, x, y, centerX, centerY, ddpX, ddpY);
            for(int32 i = 0; i < collision.count; i++) {
                ASSERT((gameState->frameCollisionCount + 1) <= 1024);
                gameState->frameCollisions[gameState->frameCollisionCount++] = collision.packets[i];
            }

        }
    }

    // Collision Resolution
    SortCollisionPacket(gameState->frameCollisions, gameState->frameCollisionCount);
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

    AdjustmentSensor sensor = AdjustCollisionWithTile(gameState, minX, maxX, minY, maxY,
                                                      centerX, centerY, inputX, inputY);

    // TODO: update the velocity intead of change the position directly
    if(inputX != 0.0f && inputY != 0.0f) {
    }
    else if(inputX != 0.0f) {
        if(sensor.mHit == false) {
            if(sensor.lHit) {
                centerY += 1.0f * 0.075f * 60.0f * dt;
            }
            if(sensor.rHit) { 
                centerY -= 1.0f * 0.075f * 60.0f * dt;
            }
        }
    }
    else if(inputY != 0.0f) {
        if(sensor.mHit == false) {
            if(sensor.lHit) {
                centerX += 1.0f * 0.075f * 60.0f * dt;
            }
            if(sensor.rHit) { 
                centerX -= 1.0f * 0.075f * 60.0f * dt;
            }
        }
    }

    gameState->heroX = centerX - SPRITE_SIZE*0.5f;
    gameState->heroY = centerY - SPRITE_SIZE;

    gameState->heroX += ddpX;
    gameState->heroY += ddpY;

 
    // Rendering Code ...
    DrawRect(backBuffer, 0, 0, backBuffer->width, backBuffer->height, 0xFFAABBAA);

    // Draw the tilemap
    for(int32 y = 0; y < gameState->tilemap.height; y++) {
        for(int32 x = 0; x < gameState->tilemap.width; x++) {
            uint32 tile = gameState->tilemap.tiles[y * gameState->tilemap.width + x];
            UV uv = gameState->tilemapUVs[tile];
            
            DrawRectTextureUV(backBuffer,
                              x * SPRITE_SIZE*MetersToPixels,
                              y * SPRITE_SIZE*MetersToPixels,
                              SPRITE_SIZE*MetersToPixels, SPRITE_SIZE*MetersToPixels,
                              uv.umin, uv.vmin, uv.umax, uv.vmax, 
                              gameState->tilemapTexture);

        }
    }
#if 0
   for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {
            
            uint32 tile = gameState->tiles[y * gameState->tilesCountX + x];
            
            if(tile == TILE_COLLISION_TYPE_NO_COLLISION) continue;
            
            DEBUG_DrawCollisionTile(backBuffer, tile, x, y);

        }
   }
#endif

    DrawRectTexture(backBuffer, 
                    gameState->heroX*MetersToPixels,
                    gameState->heroY*MetersToPixels,
                    SPRITE_SIZE*MetersToPixels,
                    SPRITE_SIZE*1.5*MetersToPixels,
                    gameState->heroTexture);  

#if 0
    centerX = gameState->heroX + SPRITE_SIZE*0.5f;
    centerY = gameState->heroY + SPRITE_SIZE;
    DrawDebugRect(backBuffer,
                  (centerX - 0.45f)*MetersToPixels,
                  (centerY - 0.45f)*MetersToPixels,
                  0.9f*MetersToPixels, 0.9f*MetersToPixels,
                  0xFFFFFF00);
#endif
   
}
