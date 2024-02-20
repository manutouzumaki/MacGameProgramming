//
//  mac_renderer.cpp
//
//
//  Created by Manuel Cabrerizo on 16/02/2024.
//
//
static const NSUInteger MaxFramesInFlight = 3;

struct MacRenderer {
    // variables for initialize  metal
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> renderPipelineState;

    // variables for the software renderer
    id<MTLTexture> textures[MaxFramesInFlight];
    MTLTextureDescriptor *textureDesc;
    id<MTLBuffer> vertices;
    NSUInteger numVertices;
    uint32 *backBuffer;
    size_t backBufferSize;

    // CPU - GPU Synchronization
    dispatch_semaphore_t inFlightSemaphore;
    NSUInteger currentTexture;
};

void InilializeSoftwareRenderer(MacRenderer *renderer, int32 windowWidth, int32 windowHeight) {
    
    static const float32 halfWidth = 1.0;
    static const float32 halfHeight = 1.0;
    // Set up a simple MTLBuffer with the vertices, including position and texture coordinates
    static const AAPLVertex quadVertices[] =
    {
        // Pixel positions, Texture coordinates
        { {  halfWidth,  -halfHeight },  { 1.f, 1.f } },
        { { -halfWidth,  -halfHeight },  { 0.f, 1.f } },
        { { -halfWidth,   halfHeight },  { 0.f, 0.f } },

        { {  halfWidth,  -halfHeight },  { 1.f, 1.f } },
        { { -halfWidth,   halfHeight },  { 0.f, 0.f } },
        { {  halfWidth,   halfHeight },  { 1.f, 0.f } },
    };

    // Create a vertex buffer, and initialize it with the vertex data.
    renderer->vertices = [renderer->device newBufferWithBytes:quadVertices
                                    length:sizeof(quadVertices)
                                   options:MTLResourceStorageModeShared];
    // Calculate the number of vertices by dividing the byte length by the size of each vertex
    renderer->numVertices = sizeof(quadVertices) / sizeof(AAPLVertex);



    renderer->textureDesc = [[MTLTextureDescriptor alloc] init];
    
    // Indicate that each pixel has a blue, green, red, and alpha channel, where each channel is
    // an 8-bit unsigned normalized value (i.e. 0 maps to 0.0 and 255 maps to 1.0)
    renderer->textureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Set the pixel dimensions of the texture
    renderer->textureDesc.width = windowWidth;
    renderer->textureDesc.height = windowHeight;
    
    // alloc memory for the color buffer
    renderer->backBufferSize = sizeof(uint32) * renderer->textureDesc.width * renderer->textureDesc.height;
    renderer->backBuffer = (uint32 *)mmap(0, renderer->backBufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0); 
    memset(renderer->backBuffer, 0, renderer->backBufferSize);

    // Create the texture from the device by using the descriptor
    for(int32 i = 0; i < MaxFramesInFlight; i++) {
        renderer->textures[i] = [renderer->device newTextureWithDescriptor:renderer->textureDesc];
    }
}

void DrawSoftwareRenderer(MacRenderer *renderer, MTKView *view) {
    
    // Wait to ensure only `MaxFramesInFlight` number of frames are getting processed
    // by any stage in the Metal pipeline (CPU, GPU, Metal, Drivers, etc.).
    dispatch_semaphore_wait(renderer->inFlightSemaphore, DISPATCH_TIME_FOREVER);

        // Iterate through the Metal buffers, and cycle back to the first when you've written to the last.
    renderer->currentTexture = (renderer->currentTexture + 1) % MaxFramesInFlight;
    
    
    // Update the backBuffer
    NSUInteger bytesPerRow = 4 * renderer->textureDesc.width;
    
    MTLRegion region = {
        { 0, 0, 0 }, // MTLOrigin
        {renderer->textureDesc.width,
         renderer->textureDesc.height,
         1} // MTLSize
    };
    [renderer->textures[renderer->currentTexture] replaceRegion:region
                                                    mipmapLevel:0
                                                      withBytes: renderer->backBuffer
                                                    bytesPerRow:bytesPerRow];

    
    // Renderer
    id<MTLCommandBuffer> commandBuffer = [renderer->commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if(renderPassDescriptor != nil) {
        
        id<MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder setRenderPipelineState:renderer->renderPipelineState];
        
        [renderEncoder setVertexBuffer:renderer->vertices
                                offset:0
                              atIndex:AAPLVertexInputIndexVertices];
        
        [renderEncoder setFragmentTexture:renderer->textures[renderer->currentTexture]
                                  atIndex:AAPLTextureIndexBaseColor];
        
        // Draw the triangles.
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart:0
                          vertexCount:renderer->numVertices];

        [renderEncoder endEncoding];

        // Schedule a present once the framebuffer is complete using the current drawable
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    // Add a completion handler that signals `_inFlightSemaphore` when Metal and the GPU have fully
    // finished processing the commands that were encoded for this frame.
    // This completion indicates that the dynamic buffers that were written-to in this frame, are no
    // longer needed by Metal and the GPU; therefore, the CPU can overwrite the buffer contents
    // without corrupting any rendering operations.
    __block dispatch_semaphore_t block_semaphore = renderer->inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
         dispatch_semaphore_signal(block_semaphore);
     }];


    // Finalize rendering here & push the command buffer to the GPU
    [commandBuffer commit];
}

void ShutdownSoftwareRenderer(MacRenderer *renderer) {
    [renderer->textureDesc dealloc];
    for(int32 i = 0; i < MaxFramesInFlight; i++) {
        [renderer->textures[i] release];
    }
    [renderer->vertices release];
    if(renderer->backBuffer) {
        munmap(renderer->backBuffer, renderer->backBufferSize);
        renderer->backBuffer = nullptr;
    }
}

bool InilializeMetal(MacRenderer *renderer) {
    // Create the device and the command queue
    renderer->device = MTLCreateSystemDefaultDevice();
    renderer->commandQueue = [renderer->device newCommandQueue];
    
    // Load the shaders
    NSError *error;
    NSString *libraryPath = [[NSBundle mainBundle] pathForResource: @"Shaders" ofType: @"metallib"];
    if(libraryPath == nil) {
        NSLog(@"Error load shader library");
        return false;
    }

    id<MTLLibrary> mtlLibrary = [renderer->device newLibraryWithFile: libraryPath
                                                               error: &error];
    if(!mtlLibrary) {
        NSLog(@"ERROR: creating Metal Library: %@", error);
        return false;
    }
    
    id<MTLFunction> vertShader = [mtlLibrary newFunctionWithName: @"vertMain"];
    id<MTLFunction> fragShader = [mtlLibrary newFunctionWithName: @"fragMain"];
    
    
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.label                           = @"MyPipeline";
    pipelineDescriptor.vertexFunction                  = vertShader;
    pipelineDescriptor.fragmentFunction                = fragShader;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    renderer->renderPipelineState = [renderer->device newRenderPipelineStateWithDescriptor: pipelineDescriptor error: &error];
    if(!renderer->renderPipelineState)
    {
        NSLog(@"ERROR: Failed aquiring pipeline state: %@", error);
        return false;
    }
    
    // Free the pipeline desc, library and theh shader funtions
    [pipelineDescriptor dealloc];
    [vertShader release];
    [fragShader release];
    [mtlLibrary release];

    renderer->inFlightSemaphore = dispatch_semaphore_create(MaxFramesInFlight);
    renderer->currentTexture = 0;
    
    return true;
}

void ShutdownMetal(MacRenderer *renderer) {
    NSLog(@"Shutingdown Metal");
    [renderer->device release];
    [renderer->commandQueue release];
    [renderer->renderPipelineState release];
}
