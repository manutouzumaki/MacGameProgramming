Entity *CreateEntity(GameState *gameState) {

    static uint32 EntityUID = 0;

    Entity *entity = (Entity *)MemoryPoolAlloc(&gameState->entityPool);
    entity->uid = EntityUID++;

    if(gameState->entities == nullptr) {
        entity->next = nullptr;
        entity->prev = nullptr;
        gameState->entities = entity;
    }
    else {
        entity->prev = nullptr;
        entity->next = gameState->entities;
        entity->next->prev = entity;
        
        gameState->entities = entity;
    }
    return entity;
}

Entity *CreatePlayer(GameState *gameState) {
    Entity *entity = CreateEntity(gameState); 
    entity->type = ENTITY_TYPE_PLAYER; 
    entity->pos = Vec2();
    entity->vel = Vec2();
    entity->dim = Vec2(0.9f, 0.9f);
    entity->spriteDim = Vec2(SPRITE_SIZE, SPRITE_SIZE*1.5);
    return entity;
}

void RemoveEntity(GameState *gameState, Entity *entity) {
    // remove it from the free list
    if(entity->prev != nullptr) {
        entity->prev->next = entity->next;
    }
    if(entity->next != nullptr) {
        entity->next->prev = entity->prev;
    }
    // free the memory block
    MemoryPoolRelease(&gameState->entityPool, entity); 
}

uint32 GetEntityCount(GameState *gameState) {
    return gameState->entityPool.elementUsed;
}

void MoveEntity(GameState *gameState, Entity *entity, float32 inputX, float32 inputY, float32 dt) {

    float32 centerX = entity->pos.x + (entity->spriteDim.x * 0.5f);
    float32 centerY = entity->pos.y + (entity->spriteDim.y * 0.5f);

    float32 ddpX = entity->vel.x;
    float32 ddpY = entity->vel.y;

    // check simple collisions
    gameState->frameCollisionCount = 0;
    
    // olny check the posible tiles, not the entire tilemap
    Vec2 hDim = entity->dim * 0.5f;
    AABB oldP;
    oldP.min = Vec2(centerX - hDim.x, centerY - hDim.y); 
    oldP.max = Vec2(centerX + hDim.x, centerY + hDim.y); 

    AABB newP;
    newP.min = Vec2(centerX + ddpX - hDim.x, centerY + ddpY - hDim.y); 
    newP.max = Vec2(centerX + ddpX + hDim.x, centerY + ddpY + hDim.y); 

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
            CollisionTile collision = GenerateCollisionTile(tileType, x, y, centerX, centerY, ddpX, ddpY, hDim.x, hDim.y);
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
        aabb.min = Vec2(collision.x - hDim.x, collision.y - hDim.y);
        aabb.max = Vec2(collision.x + collision.sizeX + hDim.x, collision.y + collision.sizeY + hDim.y);

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
#if 1
    AdjustmentSensor sensor = AdjustCollisionWithTile(gameState, minX, maxX, minY, maxY,
                                                      centerX, centerY, entity->dim.x, inputX, inputY);

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
#endif

    entity->pos.x = centerX - (entity->spriteDim.x * 0.5f);
    entity->pos.y = centerY - (entity->spriteDim.y * 0.5f);

    entity->pos.x += ddpX;
    entity->pos.y += ddpY;

    // Clear forces for next frame
    entity->vel = Vec2();
}
