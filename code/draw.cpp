//
//  draw.cpp
//
//
//  Created by Manuel Cabrerizo on 04/03/2024.
//

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


void DrawRectTextureUV(GameBackBuffer *buffer, int32 x, int32 y, int32 width, int32 height,
                       float32 umin, float32 vmin, float32 umax, float32 vmax,
                       Texture texture) {
    int32 minX = MAX(x, 0);
    int32 minY = MAX(y, 0);
    int32 maxX = MIN(x + width, buffer->width);
    int32 maxY = MIN(y + height, buffer->height);

    int32 offsetX = -MIN(x, 0);
    int32 offsetY = -MIN(y, 0);

    int32 startX = (int32)(umin * texture.width);
    int32 startY = (int32)(vmin * texture.height);
    int32 endX = (int32)(umax * texture.width);
    int32 endY = (int32)(vmax * texture.height);
    
    uint32 *pixels = (uint32 *)buffer->data;
    for(int32 y = minY; y < maxY; y++) {

        float32 ty = ((float32)(y - minY + offsetY) / (float32)height);
        int32 texY = (1.0f - ty) * startY + ty * endY; 
        

        for(int32 x = minX; x < maxX; x++) {
            
            float32 tx = ((float32)(x - minX + offsetX) / (float32)width);
            int32 texX = (1.0f - tx) * startX + tx * endX; 

        
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

void DEBUG_DrawCollisionTile(GameBackBuffer *backBuffer, uint32 tile, int x, int y) {
    
    int32 count = 0;

    switch(tile) {
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

    float32 sizeX = (float32)SPRITE_SIZE / (float32)count;
    float32 sizeY = (float32)SPRITE_SIZE / (float32)count;

    switch(tile) {
        case TILE_COLLISION_TYPE_16x16: {
            float32 posX = x;
            float32 posY = y;
            DrawDebugRect(backBuffer,
                          posX * SPRITE_SIZE*MetersToPixels,
                          posY * SPRITE_SIZE*MetersToPixels,
                          sizeX*MetersToPixels, sizeY*MetersToPixels,
                          0xFF00FF00);                       
        } break;
        case TILE_COLLISION_TYPE_8x8_L_U:
        case TILE_COLLISION_TYPE_4x4_L_U: {
            float32 posX = x + (count - 1)*sizeX;
            float32 posY = y;
            for(int32 j = 0; j < count; j++) {
                for(int32 i = 0; i < j + 1; i++) {                
                    DrawDebugRect(backBuffer,
                                  posX * SPRITE_SIZE*MetersToPixels,
                                  posY * SPRITE_SIZE*MetersToPixels,
                                  sizeX*MetersToPixels, sizeY*MetersToPixels,
                                  0xFF00FF00);
                    posX -= sizeX;
                }
                posX = x + (count - 1)*sizeX;
                posY += sizeY;
            }
        } break;
        case TILE_COLLISION_TYPE_8x8_R_U:
        case TILE_COLLISION_TYPE_4x4_R_U: {
            float32 posX = x;
            float32 posY = y;
            for(int32 j = 0; j < count; j++) {
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
        } break;
        case TILE_COLLISION_TYPE_8x8_L_D:
        case TILE_COLLISION_TYPE_4x4_L_D: {
            float32 posX = x + (count - 1)*sizeX;
            float32 posY = y;
            for(int32 j = 0; j < count; j++) {
                for(int32 i = 0; i < count - j; i++) {                
                    DrawDebugRect(backBuffer,
                                  posX * SPRITE_SIZE*MetersToPixels,
                                  posY * SPRITE_SIZE*MetersToPixels,
                                  sizeX*MetersToPixels, sizeY*MetersToPixels,
                                  0xFF00FF00);
                    posX -= sizeX;
                }
                posX = x + (count - 1)*sizeX;
                posY += sizeY;
            }
        } break;
        case TILE_COLLISION_TYPE_8x8_R_D:
        case TILE_COLLISION_TYPE_4x4_R_D: {
            float32 posX = x;
            float32 posY = y;
            for(int32 j = 0; j < count; j++) {
                for(int32 i = 0; i < count - j; i++) {                
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
        } break;
    }
}
