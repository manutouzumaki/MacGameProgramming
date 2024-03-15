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

    gameState->assetsArena = ArenaCreate(memory, MB(50));
    gameState->networkArena = ArenaCreate(memory, MB(10));

    gameState->entityPool = MemoryPoolCreate(memory, MaxEntityCount, sizeof(Entity));
    gameState->networkToEntity.Initialize(&gameState->networkArena, 128);

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

    gameState->totalGameTime = 0;
    gameState->inputSamplesCount = 0;
    gameState->lastTimeStamp = 0;

    gameState->socket = UDPSocketCreate();
    gameState->address = UDPAddresCreate(IP(127, 0, 0, 1), 0);
    UDPSocketBind(&gameState->socket, &gameState->address);
    UDPSocketSetNonBlockingMode(&gameState->socket, true);

    gameState->clientState = CLIENT_STATE_HELLO;
    gameState->sendAddress = UDPAddresCreate(IP(127, 0, 0, 1), 35000);
}

void GameUpdateAndRender(Memory *memory, GameSound *sound, GameInput *input, GameBackBuffer *backBuffer) {

    GameState *gameState = (GameState *)memory->data;
    float32 dt = input->deltaTime;
    if(gameState->clientState == CLIENT_STATE_HELLO) {
        char buffer[1200];
        if(gameState->timePassFromLastInputPacket >= 1) {

            MemoryStream outStream = MemoryStreamCreate(buffer, 1200);
            MemoryStreamWrite(&outStream, (void *)&PacketHeader, sizeof(int32));
            MemoryStreamWrite(&outStream, (void *)&PacketTypeHello, sizeof(int32));

            int sentBytes = UDPSocketSendTo(&gameState->socket, buffer, 1200, &gameState->sendAddress);
            if (sentBytes != 1200) {
                printf( "failed to send Hello packet\n" );
            }
            else {
                printf( "Hello Packet send\n" );
            }

            gameState->timePassFromLastInputPacket -= 1;
        }
        gameState->timePassFromLastInputPacket += dt;

        // TODO: check for the welcome packe to arrive ...
        UDPAddress fromAddress;
        int32 bytes = UDPSocketReceiveFrom(&gameState->socket, buffer, 1200, &fromAddress);
        if(bytes > 0) {
            MemoryStream inStream = MemoryStreamCreate(buffer, 1200);            
            int32 header;
            MemoryStreamRead(&inStream, &header, sizeof(int32));
            if(header == PacketHeader) {
                int32 type;
                MemoryStreamRead(&inStream, &type, sizeof(int32));
                if(type == PacketTypeWelcome) {
                    uint32 networkID;
                    MemoryStreamRead(&inStream, &networkID, sizeof(uint32));
                    gameState->entity = CreatePlayer(gameState);
                    gameState->entity->uid = networkID;
                    gameState->networkToEntity.Add(networkID, gameState->entity);
                    gameState->clientState = CLIENT_STATE_WELCOMED;
                    gameState->timePassFromLastInputPacket = 0;
                    printf("Welcome Packet Recived\n");
                    printf("Client conected to the server\n");
                }
            }

        }
    
    }
    else if(gameState->clientState == CLIENT_STATE_WELCOMED) {

        bool sendPacketThisFrame = false;

        if(input->controllers[0].A.endedDown) {
            sound->Restart(gameState->missionCompleted);
            sound->Play(gameState->missionCompleted);
        }

        Entity *hero = gameState->entity;
        float32 inputX = 0;
        float32 inputY = 0;
        if(input->controllers[0].left.endedDown) {
            hero->vel.x -= 1;
            inputX -= 1;
            sendPacketThisFrame = true;
        }
        if(input->controllers[0].right.endedDown) {
            hero->vel.x += 1;
            inputX += 1;
            sendPacketThisFrame = true;
        }
        if(input->controllers[0].up.endedDown) {
            hero->vel.y -= 1;
            inputY -= 1;
            sendPacketThisFrame = true;
        }
        if(input->controllers[0].down.endedDown) {
            hero->vel.y += 1;
            inputY += 1;
            sendPacketThisFrame = true;
        }

        Normalize(&hero->vel);
        hero->vel *= 0.075f * 60.0f * dt;

        // Sample inputs states
        {
            if(gameState->timePassFromLastInputSampled >= TimeBetweenInputSamples) {
               
                gameState->inputSamples[2] = gameState->inputSamples[1];
                gameState->inputSamples[1] = gameState->inputSamples[0];
                
                float32 deltaTime = gameState->lastTimeStamp >= 0.f ? gameState->totalGameTime - gameState->lastTimeStamp : 0.f;

                gameState->inputSamples[0].inputX = inputX;
                gameState->inputSamples[0].inputY = inputY;
                gameState->inputSamples[0].vel = hero->vel;
                gameState->inputSamples[0].deltaTime = deltaTime;
                gameState->inputSamples[0].timeStamp = gameState->totalGameTime;
                    
                gameState->lastTimeStamp = gameState->totalGameTime;
                
                gameState->inputSamplesCount = MIN(3, gameState->inputSamplesCount + 1);
                
                gameState->timePassFromLastInputSampled -= TimeBetweenInputSamples;
            }
            gameState->timePassFromLastInputSampled += dt;
        }


        char buffer[1200];
        // Send input Messages
        {
            if(gameState->timePassFromLastInputPacket >= TimeBetweenInputPackets) {
                MemoryStream outStream = MemoryStreamCreate(buffer, 1200);
                MemoryStreamWrite(&outStream, (void *)&PacketHeader, sizeof(int32));
                MemoryStreamWrite(&outStream, (void *)&PacketTypeState, sizeof(int32));
                MemoryStreamWrite(&outStream, (void *)&gameState->entity->uid, sizeof(uint32));
                MemoryStreamWrite(&outStream, (void *)&gameState->inputSamplesCount, sizeof(int32));
                MemoryStreamWrite(&outStream, (void *)&gameState->inputSamples[0], sizeof(InputState));
                MemoryStreamWrite(&outStream, (void *)&gameState->inputSamples[1], sizeof(InputState));
                MemoryStreamWrite(&outStream, (void *)&gameState->inputSamples[2], sizeof(InputState));

                if(sendPacketThisFrame) {
                    int sentBytes = UDPSocketSendTo(&gameState->socket, buffer, 1200, &gameState->sendAddress);
                    if (sentBytes != 1200) {
                        printf( "failed to send packet\n" );
                    }

                    printf( "Packet send\n" );
                }

                gameState->timePassFromLastInputPacket -= TimeBetweenInputPackets;
            }
            gameState->timePassFromLastInputPacket += dt;
        }

        UDPAddress fromAddress;
        int32 bytes = UDPSocketReceiveFrom(&gameState->socket, buffer, 1200, &fromAddress);
        if(bytes > 0) {
            MemoryStream inStream = MemoryStreamCreate(buffer, 1200);            

            int32 entitiesToUpdateCount;
            MemoryStreamRead(&inStream, & entitiesToUpdateCount, sizeof(int32));

            for(int32 i = 0; i < entitiesToUpdateCount; ++i) {
                int32 header;
                MemoryStreamRead(&inStream, &header, sizeof(int32));
                if(header == PacketHeader) {
                    int32 type;
                    MemoryStreamRead(&inStream, &type, sizeof(int32));
                    if(type == PacketTypeState) {
                        uint32 networkID;
                        MemoryStreamRead(&inStream, &networkID, sizeof(uint32));
                        Vec2 pos, vel;
                        MemoryStreamRead(&inStream, &pos, sizeof(Vec2));
                        MemoryStreamRead(&inStream, &vel, sizeof(Vec2));
                        
                        Entity *entity = gameState->networkToEntity.Get(networkID);
                        if(entity) {
                            entity->pos = pos;
                            entity->vel = vel;
                        }
                        else {
                            // TODO: add the new entity
                            Entity *newEntity = CreatePlayer(gameState);
                            newEntity->uid = networkID;
                            newEntity->pos = pos;
                            newEntity->vel = vel;
                            gameState->networkToEntity.Add(networkID, newEntity);
                            
                        }
                    }
                }           
            }

        }
    }

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

   for(int32 y = 0; y < gameState->tilesCountY; y++) {
        for(int32 x = 0; x < gameState->tilesCountX; x++) {
            
            uint32 tile = gameState->tiles[y * gameState->tilesCountX + x];
            
            if(tile == TILE_COLLISION_TYPE_NO_COLLISION) continue;
            
            DEBUG_DrawCollisionTile(backBuffer, tile, x, y);

        }
   }

    Entity *hero = gameState->entities;
    while(hero) { 

        DrawRectTexture(backBuffer, 
                        hero->pos.x*MetersToPixels,
                        hero->pos.y*MetersToPixels,
                        hero->spriteDim.x*MetersToPixels,
                        hero->spriteDim.y*MetersToPixels,
                        gameState->heroTexture);  

        float32 centerX = hero->pos.x + (hero->spriteDim.x * 0.5f);
        float32 centerY = hero->pos.y + (hero->spriteDim.y * 0.5f);
        DrawDebugRect(backBuffer,
                      (centerX - hero->dim.x * 0.5f)*MetersToPixels,
                      (centerY - hero->dim.y * 0.5f)*MetersToPixels,
                      hero->dim.x*MetersToPixels, hero->dim.y*MetersToPixels,
                      0xFFFFFF00);
        hero = hero->next;
    }
    
    gameState->totalGameTime += dt;
   
}
