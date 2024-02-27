//
//  tilemap.cpp
//
//
//  Created by Manuel Cabrerizo on 27/02/2024.
//

CollisionTile GenerateCollisionTileSQ(int32 x, int32 y, float32 centerX, float32 centerY, float32 ddpX, float32 ddpY) {
    CollisionTile result;
    result.count = 0;

    // TODO: pass the size of the sprite that is going to collide with this
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
        result.packets[result.count++] = collision;
    }

    return result;
}

CollisionTile GenerateCollisionTileRU(int32 count, int32 x, int32 y,
                                      float32 centerX, float32 centerY,
                                      float32 ddpX, float32 ddpY) {
    CollisionTile result;
    result.count = 0;

    float32 sizeX = (float32)SPRITE_SIZE / (float32)count;
    float32 sizeY = (float32)SPRITE_SIZE / (float32)count;
    float32 posX = x;
    float32 posY = y;
    for(int32 j = 0; j < count; j++) {
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
                result.packets[result.count++] = collision;
            }

            posX += sizeX;
        }
        posX = x;
        posY += sizeY;
    }

    return result;
}

CollisionTile GenerateCollisionTileRD(int32 count, int32 x, int32 y,
                                      float32 centerX, float32 centerY,
                                      float32 ddpX, float32 ddpY) {
    CollisionTile result;
    result.count = 0;

    float32 sizeX = (float32)SPRITE_SIZE / (float32)count;
    float32 sizeY = (float32)SPRITE_SIZE / (float32)count;
    float32 posX = x;
    float32 posY = y;
    for(int32 j = 0; j < count; j++) {
        for(int32 i = 0; i < count - j; i++) {                

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
                result.packets[result.count++] = collision;
            }

            posX += sizeX;
        }
        posX = x;
        posY += sizeY;
    }

    return result;
}

CollisionTile GenerateCollisionTileLU(int32 count, int32 x, int32 y,
                                      float32 centerX, float32 centerY,
                                      float32 ddpX, float32 ddpY) {
    CollisionTile result;
    result.count = 0;

    float32 sizeX = (float32)SPRITE_SIZE / (float32)count;
    float32 sizeY = (float32)SPRITE_SIZE / (float32)count;
    float32 posX = x + (count - 1)*sizeX;
    float32 posY = y;
    for(int32 j = 0; j < count; j++) {
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
                result.packets[result.count++] = collision;
            }

            posX -= sizeX;
        }
        posX = x + (count - 1)*sizeX;
        posY += sizeY;
    }

    return result;
}

CollisionTile GenerateCollisionTileLD(int32 count, int32 x, int32 y,
                                      float32 centerX, float32 centerY,
                                      float32 ddpX, float32 ddpY) {
    CollisionTile result;
    result.count = 0;

    float32 sizeX = (float32)SPRITE_SIZE / (float32)count;
    float32 sizeY = (float32)SPRITE_SIZE / (float32)count;
    float32 posX = x + (count - 1)*sizeX;
    float32 posY = y;
    for(int32 j = 0; j < count; j++) {
        for(int32 i = 0; i < count - j; i++) {                

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
                result.packets[result.count++] = collision;
            }

            posX -= sizeX;
        }
        posX = x + (count - 1)*sizeX;
        posY += sizeY;
    }

    return result;
}

CollisionTile GenerateCollisionTile(TileCollisionType tileType, int32 x, int32 y,
                                    float32 centerX, float32 centerY,
                                    float32 ddpX, float32 ddpY) {
    int32 count = 0;
    switch(tileType) {
        case TILE_COLLISION_TYPE_NO_COLLISION: break;
        case TILE_COLLISION_TYPE_16x16: {
            count = 1;
        } break;
        case TILE_COLLISION_TYPE_8x8_L_U:
        case TILE_COLLISION_TYPE_8x8_R_U:
        case TILE_COLLISION_TYPE_8x8_L_D:
        case TILE_COLLISION_TYPE_8x8_R_D: {
            count = 2;
        } break;
        case TILE_COLLISION_TYPE_4x4_L_U:
        case TILE_COLLISION_TYPE_4x4_R_U:
        case TILE_COLLISION_TYPE_4x4_L_D:
        case TILE_COLLISION_TYPE_4x4_R_D: {
            count = 4;
        } break;
    }

    switch(tileType) {
        case TILE_COLLISION_TYPE_NO_COLLISION: break;
        case TILE_COLLISION_TYPE_16x16: {
            return GenerateCollisionTileSQ(x, y, centerX, centerY, ddpX, ddpY);
        } break;
        case TILE_COLLISION_TYPE_8x8_L_U:
        case TILE_COLLISION_TYPE_4x4_L_U: {
            return GenerateCollisionTileLU(count, x, y, centerX, centerY, ddpX, ddpY);
        } break;
        case TILE_COLLISION_TYPE_8x8_R_U:
        case TILE_COLLISION_TYPE_4x4_R_U: {
            return GenerateCollisionTileRU(count, x, y, centerX, centerY, ddpX, ddpY);
        } break;
        case TILE_COLLISION_TYPE_8x8_L_D:
        case TILE_COLLISION_TYPE_4x4_L_D: {
            return GenerateCollisionTileLD(count, x, y, centerX, centerY, ddpX, ddpY);
        } break;
        case TILE_COLLISION_TYPE_8x8_R_D:
        case TILE_COLLISION_TYPE_4x4_R_D: {
            return GenerateCollisionTileRD(count, x, y, centerX, centerY, ddpX, ddpY);
        } break;
    }
    
    CollisionTile zero;
    zero.count = 0;
    return zero;

}

