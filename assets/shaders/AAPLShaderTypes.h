//
//  AAPLShaderTypes.h
//  VoxelSpaceEngine
//
//  Created by Manuel Cabrerizo on 07/01/2023.
//

#ifndef AAPLShaderTypes_h
#define AAPLShaderTypes_h

#include <simd/simd.h>

// TODO: change the neame of this struct and leave only the usefull stuff

typedef enum AAPLVertexInputIndex
{
    AAPLVertexInputIndexVertices = 0,
    AAPLVertexInputIndexUniforms = 1,
} AAPLVertexInputIndex;

// Texture index values shared between shader and C code to ensure Metal shader buffer inputs match
//   Metal API texture set calls
typedef enum AAPLTextureIndex
{
    AAPLTextureIndexBaseColor = 0,
} AAPLTextureIndex;

typedef struct
{
    // Positions in pixel space (i.e. a value of 100 indicates 100 pixels from the origin/center)
    vector_float2 position;

    // 2D texture coordinate
    vector_float2 textureCoordinate;
} AAPLVertex;

typedef struct
{
    float scale;
    vector_uint2 viewportSize;
} AAPLUniforms;

#endif /* AAPLShaderTypes_h */
