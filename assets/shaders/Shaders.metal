//
//  AAPLShaders.metal
//  VoxelSpaceEngine
//
//  Created by Manuel Cabrerizo on 07/01/2023.
//

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Include header shared between this Metal shader code and C code executing Metal API commands
#include "AAPLShaderTypes.h"

// Vertex shader outputs and per-fragment inputs
struct RasterizerData
{
    float4 position [[position]];
    float2 textureCoordinate;
};

vertex RasterizerData
vertMain(uint vertexID [[ vertex_id ]],
         constant AAPLVertex *vertexArray [[ buffer(AAPLVertexInputIndexVertices) ]],
         constant AAPLUniforms &uniforms  [[ buffer(AAPLVertexInputIndexUniforms) ]])

{
    RasterizerData out;

    float2 pixelSpacePosition = vertexArray[vertexID].position.xy;
    out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.position.xy = pixelSpacePosition;
    out.textureCoordinate = vertexArray[vertexID].textureCoordinate;

    return out;
}


// Fragment function
fragment float4
fragMain(RasterizerData in [[stage_in]],
         texture2d<half> colorTexture [[ texture(AAPLTextureIndexBaseColor) ]])
{
    constexpr sampler textureSampler (mag_filter::nearest,
                                      min_filter::nearest);
    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    return float4(colorSample);
}

