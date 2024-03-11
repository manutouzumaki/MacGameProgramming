//
//  handmade.cpp
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

    gameState->entityPool = MemoryPoolCreate(memory, MaxEntityCount, sizeof(Entity));

    gameState->hero = CreatePlayer(gameState);

    gameState->oliviaRodrigo = sound->Load(&gameState->assetsArena, "test", false, true);
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
}

void GameUpdateAndRender(Memory *memory, GameSound *sound, GameInput *input, GameBackBuffer *backBuffer) {

    GameState *gameState = (GameState *)memory->data;

    float32 dt = input->deltaTime;

    if(input->controllers[0].A.endedDown) {
        sound->Restart(gameState->missionCompleted);
        sound->Play(gameState->missionCompleted);
    }

    Entity *hero = gameState->hero;
    float32 inputX = 0;
    float32 inputY = 0;
    if(input->controllers[0].left.endedDown) {
        hero->vel.x -= 1;
        inputX -= 1;
    }
    if(input->controllers[0].right.endedDown) {
        hero->vel.x += 1;
        inputX += 1;
    }
    if(input->controllers[0].up.endedDown) {
        hero->vel.y -= 1;
        inputY -= 1;
    }
    if(input->controllers[0].down.endedDown) {
        hero->vel.y += 1;
        inputY += 1;
    }

    Normalize(&hero->vel);
    hero->vel *= 0.075f * 60.0f * dt;

    MoveEntity(gameState, hero, inputX, inputY, dt);
     
 
    // Rendering Code ...
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
#if 1
   for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {
            
            uint32 tile = gameState->tiles[y * gameState->tilesCountX + x];
            
            if(tile == TILE_COLLISION_TYPE_NO_COLLISION) continue;
            
            DEBUG_DrawCollisionTile(backBuffer, tile, x, y);

        }
   }
#endif

    DrawRectTexture(backBuffer, 
                    hero->pos.x*MetersToPixels,
                    hero->pos.y*MetersToPixels,
                    hero->spriteDim.x*MetersToPixels,
                    hero->spriteDim.y*MetersToPixels,
                    gameState->heroTexture);  

#if 1
    float32 centerX = hero->pos.x + (hero->spriteDim.x * 0.5f);
    float32 centerY = hero->pos.y + (hero->spriteDim.y * 0.5f);
    DrawDebugRect(backBuffer,
                  (centerX - hero->dim.x * 0.5f)*MetersToPixels,
                  (centerY - hero->dim.y * 0.5f)*MetersToPixels,
                  hero->dim.x*MetersToPixels, hero->dim.y*MetersToPixels,
                  0xFFFFFF00);
#endif
   
}
