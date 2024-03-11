//
//  collision.cpp
//
//
//  Created by Manuel Cabrerizo on 27/02/2024.
//

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

void SortCollisionPacket(CollisionPacket *collisions, int32 count) {

    int32 i, j;
    bool swapped;
    for (i = 0; i < count - 1; i++) {
        swapped = false;
        for (j = 0; j < count - i - 1; j++) {
            if (collisions[j].t > collisions[j + 1].t) {

                CollisionPacket tmp = collisions[j];
                collisions[j] = collisions[j + 1];
                collisions[j + 1] = tmp;

                swapped = true;
            }
        }
 
        // If no two elements were swapped by inner loop,
        // then break
        if (swapped == false)
            break;
    }

}

// link to the pass collision adjusment
void CollisionAdjusment(AABB aabbOther,
                        float32 centerX, float32 centerY,
                        float32 size,
                        float32 inputX, float32 inputY,
                        bool &lHit, bool &mHit, bool &rHit) {
    float32 buttonPressed = inputX * inputX + inputY * inputY;
    if(buttonPressed > 0.0f) {
        
        AABB sensorL;
        AABB sensorM;
        AABB sensorR;
        
        // TODO: fix the size, should not be related to the size of the collider to slide diagonaly properly
        float32 LRwidth = 0.25f * size;
        float32 Mwidth = 0.4f * size;
        float32 height = 0.125f * size;

        if(inputX != 0.0f && inputY != 0.0f) {
        }
        else if(inputX > 0.0f) {
            float32 sensorX = centerX + (size*0.5f) + (height*0.5f);
            float32 sensorY = centerY;
            sensorM.min = Vec2(sensorX - height * 0.5f, sensorY - Mwidth * 0.5f);
            sensorM.max = Vec2(sensorX + height * 0.5f, sensorY + Mwidth * 0.5f); 

            sensorX = centerX + (size*0.5f) + (height * 0.5f);
            sensorY = centerY - (size*0.5f) + (LRwidth * 0.5f);
            sensorL.min = Vec2(sensorX - height * 0.5f, sensorY - LRwidth * 0.5f);
            sensorL.max = Vec2(sensorX + height * 0.5f, sensorY + LRwidth * 0.5f);
              
            sensorX = centerX + (size*0.5f) + (height * 0.5f);
            sensorY = centerY + (size*0.5f) - (LRwidth * 0.5f);
            sensorR.min = Vec2(sensorX - height * 0.5f, sensorY - LRwidth * 0.5f);
            sensorR.max = Vec2(sensorX + height * 0.5f, sensorY + LRwidth * 0.5f);
            
            if(mHit == false)
                mHit = AABBVsAABB(sensorM, aabbOther);
            if(lHit == false)
                lHit = AABBVsAABB(sensorL, aabbOther);
            if(rHit == false)
                rHit = AABBVsAABB(sensorR, aabbOther);        
        }
        else if(inputX < 0.0f) {
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
            
            if(mHit == false)
                mHit = AABBVsAABB(sensorM, aabbOther);
            if(lHit == false)
                lHit = AABBVsAABB(sensorL, aabbOther);
            if(rHit == false)
                rHit = AABBVsAABB(sensorR, aabbOther);
        }
        else if(inputY > 0.0f) {
            float32 sensorX = centerX;
            float32 sensorY = centerY + (size*0.5f) + (height*0.5f);
            sensorM.min = Vec2(sensorX - Mwidth * 0.5f, sensorY - height * 0.5f);
            sensorM.max = Vec2(sensorX + Mwidth * 0.5f, sensorY + height * 0.5f); 

            sensorX = centerX - (size*0.5f) + (LRwidth * 0.5f);
            sensorY = centerY + (size*0.5f) + (height * 0.5f);
            sensorL.min = Vec2(sensorX - LRwidth * 0.5f, sensorY - height * 0.5f);
            sensorL.max = Vec2(sensorX + LRwidth * 0.5f, sensorY + height * 0.5f);
              
            sensorX = centerX + (size*0.5f) - (LRwidth * 0.5f);
            sensorY = centerY + (size*0.5f) + (height * 0.5f);
            sensorR.min = Vec2(sensorX - LRwidth * 0.5f, sensorY - height * 0.5f);
            sensorR.max = Vec2(sensorX + LRwidth * 0.5f, sensorY + height * 0.5f);

            if(mHit == false)
                mHit = AABBVsAABB(sensorM, aabbOther);
            if(lHit == false)
                lHit = AABBVsAABB(sensorL, aabbOther);
            if(rHit == false)
                rHit = AABBVsAABB(sensorR, aabbOther);    
        }
        else if(inputY < 0.0f) {
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

            if(mHit == false)
                mHit = AABBVsAABB(sensorM, aabbOther);
            if(lHit == false)
                lHit = AABBVsAABB(sensorL, aabbOther);
            if(rHit == false)
                rHit = AABBVsAABB(sensorR, aabbOther);        
        }
    }
}
