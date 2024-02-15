//
//  handmade_mac.mm
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//

#import <AppKit/AppKit.h>
#import <MetalKit/MetalKit.h>
#import <GameController/GameController.h>

#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

#import "../assets/shaders/AAPLShaderTypes.h"

#include "common.h"
#include "handmade.h"

#include "handmade.cpp"
#include "mac_sound_sys.cpp"

// TODO: memory managment
// TODO: add mac prefixx to the function on this file ...
// TODO: make structs for this globals

// globals for initialize  metal
static const NSUInteger gMaxInFlightBuffers = 3;
static id<MTLDevice> gDevice;
static id<MTLCommandQueue> gCommandQueue;
static id<MTLRenderPipelineState> gRenderPipelineState;

// globals for the software renderer
static id<MTLTexture> gTexture;
static MTLTextureDescriptor *gTextureDesc;
static id<MTLBuffer> gVertices;
static NSUInteger gNumvertices;
static unsigned int *gBackBuffer;

// globals for input handling
static const unsigned int gMaxControllerCount = 4;
static GCController *gControllers[gMaxControllerCount] = {};
static unsigned int gControllerConected = 0;
static GCKeyboard *gKeyboard;

static GameInput gInputs[2] = {};
static GameInput *gCurrentInput;
static GameInput *gLastInput;


void ShutdownMetal();
void ShutdownSoftwareRenderer();
void DrawSoftwareRenderer(MTKView *view);
void ProcessInput(GameInput *input);

@interface GameWindow : NSWindow
@end

@implementation GameWindow

- (void)keyDown:(NSEvent *)theEvent { /* NOTE: this removes the beeps that the keyboard does */ }

@end


// Window Delegate
@interface WindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    // Free all memory that we allocated
    // TODO: find a better way
    static bool appShutdown = false;
    if(appShutdown == false) {
        appShutdown = true;
        MacSoundSysShutdown();
        ShutdownSoftwareRenderer();
        ShutdownMetal();
    }
    
    [NSApp terminate: nil];
}
@end

// MTKView Delegate
@interface MetalViewDelegate: NSObject<MTKViewDelegate>
@end

@implementation MetalViewDelegate
// NOTE: game's render loop
- (void)drawInMTKView:(MTKView *) view {
    
    ProcessInput(gCurrentInput);
    
    GameBackBuffer gameBackBuffer;
    gameBackBuffer.data = (void *)gBackBuffer;
    gameBackBuffer.width = gTextureDesc.width;
    gameBackBuffer.height = gTextureDesc.height;
    gameBackBuffer.pitch = gTextureDesc.width * 4;
    GameUpdateAndRender(gCurrentInput, &gameBackBuffer);

    DrawSoftwareRenderer(view);
    
    GameInput *tmp = gCurrentInput;
    gCurrentInput = gLastInput;
    gLastInput = gCurrentInput;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize) size {
}
@end

@interface GameInputController: NSObject
- (void)controlWasConnected:(NSNotification *)notification;
- (void)controlWasDisconnected:(NSNotification *)notification;
- (void)keyboardWasConnected:(NSNotification *)notification;
- (void)keyboardWasDisconnected:(NSNotification *)notification;
@end

@implementation GameInputController
- (void)controlWasConnected:(NSNotification *)notification {
    // a controller was connected
    GCController *controller = (GCController *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nController connected\nName: %@\n", controller.vendorName];
    NSLog(@"%@", status);
    for(unsigned int i = 0; i < gMaxControllerCount; i++) {
        if(gControllers[i] == nil) {
            gControllers[i] = controller;
            gControllerConected++;
            break;
        }
    }
}

- (void)controlWasDisconnected:(NSNotification *)notification {
    // a controller was disconnected
    GCController *controller = (GCController *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nController disconnected:\n%@", controller.vendorName];
    NSLog(@"%@", status);
    
    for(unsigned int i = 0; i < gMaxControllerCount; i++) {
        if(gControllers[i] == controller) {
            gControllers[i] = nil;
            gControllerConected--;
            break;
        }
    }
}

- (void)keyboardWasConnected:(NSNotification *)notification {
    GCKeyboard *keyboard = (GCKeyboard *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nKeyboard connected\nName: %@\n", keyboard.vendorName];
    NSLog(@"%@", status);
    gKeyboard = keyboard;
}
- (void)keyboardWasDisconnected:(NSNotification *)notification {
    GCKeyboard *keyboard = (GCKeyboard *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nKeyboard disconnected:\n%@", keyboard.vendorName];
    NSLog(@"%@", status);
    gKeyboard = nil;
}

@end

void ProcessInput(GameInput *input) {
    
    for(unsigned int  i = 0; i < gMaxControllerCount; i++) {
        for(int j = 0; j < 8; j++) {
            input->controllers[i].buttons[j].endedDown = false;
        }
        
        GCController *controller = gControllers[i];
        if(controller) {
            GCExtendedGamepad *gamepad = [controller extendedGamepad];
            input->controllers[i].A.endedDown = gamepad.buttonA.isPressed;
            input->controllers[i].B.endedDown = gamepad.buttonB.isPressed;
            input->controllers[i].X.endedDown = gamepad.buttonX.isPressed;
            input->controllers[i].Y.endedDown = gamepad.buttonY.isPressed;
            input->controllers[i].left.endedDown = gamepad.dpad.left.isPressed;
            input->controllers[i].right.endedDown = gamepad.dpad.right.isPressed;
            input->controllers[i].up.endedDown = gamepad.dpad.up.isPressed;
            input->controllers[i].down.endedDown = gamepad.dpad.down.isPressed;
        }
    }
    
    GCKeyboardInput *keyboardInput = [gKeyboard keyboardInput];
    input->controllers[0].A.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeSpacebar].pressed;
    input->controllers[0].B.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeKeyB].pressed;
    input->controllers[0].X.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeKeyR].pressed;
    input->controllers[0].Y.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeKeyF].pressed;
    
    input->controllers[0].left.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeLeftArrow].pressed;
    input->controllers[0].right.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeRightArrow].pressed;
    input->controllers[0].up.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeUpArrow].pressed;
    input->controllers[0].down.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeDownArrow].pressed;
    
    int stopHere = 0;
    
}

void InilializeSoftwareRenderer() {
    
    static const float halfWidth = 1.0;
    static const float halfHeight = 1.0;
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
    gVertices = [gDevice newBufferWithBytes:quadVertices
                                    length:sizeof(quadVertices)
                                   options:MTLResourceStorageModeShared];
    // Calculate the number of vertices by dividing the byte length by the size of each vertex
    gNumvertices = sizeof(quadVertices) / sizeof(AAPLVertex);



    gTextureDesc = [[MTLTextureDescriptor alloc] init];
    
    // Indicate that each pixel has a blue, green, red, and alpha channel, where each channel is
    // an 8-bit unsigned normalized value (i.e. 0 maps to 0.0 and 255 maps to 1.0)
    gTextureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Set the pixel dimensions of the texture
    gTextureDesc.width = 800;
    gTextureDesc.height = 600;
    
    // TODO: use this function to allocate memory
    // void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t offset);
    
    // alloc memory for the color buffer
    gBackBuffer = (unsigned int *)malloc(sizeof(unsigned int) * gTextureDesc.width * gTextureDesc.height);
    memset(gBackBuffer, 0, sizeof(unsigned int) * gTextureDesc.width * gTextureDesc.height);

    for(int i = 0; i < 800*600; i++) {
        gBackBuffer[i] = 0xFF00FFBB;
    }
                
    // Create the texture from the device by using the descriptor
    gTexture = [gDevice newTextureWithDescriptor:gTextureDesc];
}

void DrawSoftwareRenderer(MTKView *view) {
    // Update the backBuffer
    NSUInteger bytesPerRow = 4 * gTexture.width;
    
    MTLRegion region = {
        { 0, 0, 0 },                                 // MTLOrigin
        {gTextureDesc.width, gTextureDesc.height, 1} // MTLSize
    };
    [gTexture replaceRegion:region
                mipmapLevel:0
                  withBytes: gBackBuffer
                bytesPerRow:bytesPerRow];
    
    // Renderer
    id<MTLCommandBuffer> commandBuffer = [gCommandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if(renderPassDescriptor != nil) {
        
        id<MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder setRenderPipelineState:gRenderPipelineState];
        
        [renderEncoder setVertexBuffer:gVertices
                                offset:0
                              atIndex:AAPLVertexInputIndexVertices];
        
        [renderEncoder setFragmentTexture:gTexture
                                  atIndex:AAPLTextureIndexBaseColor];
        
        // Draw the triangles.
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart:0
                          vertexCount:gNumvertices];

        [renderEncoder endEncoding];

        // Schedule a present once the framebuffer is complete using the current drawable
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    // Finalize rendering here & push the command buffer to the GPU
    [commandBuffer commit];
}

void ShutdownSoftwareRenderer() {
    [gTextureDesc dealloc];
    [gTexture release];
    [gVertices release];
    if(gBackBuffer) {
        // TODO: use this function to free the memory
        // int munmap(void *addr, size_t len);
        free(gBackBuffer);
        gBackBuffer = nullptr;
    }
}

bool InilializeMetal() {
    // Create the device and the command queue
    gDevice = MTLCreateSystemDefaultDevice();
    gCommandQueue = [gDevice newCommandQueue];
    
    // Load the shaders
    NSError *error;
    NSString *libraryPath = [[NSBundle mainBundle] pathForResource: @"Shaders" ofType: @"metallib"];
    id<MTLLibrary> mtlLibrary = [gDevice newLibraryWithFile: libraryPath
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
    gRenderPipelineState = [gDevice newRenderPipelineStateWithDescriptor: pipelineDescriptor
                                                                   error: &error];
    if(!gRenderPipelineState)
    {
        NSLog(@"ERROR: Failed aquiring pipeline state: %@", error);
        return false;
    }
    
    // Free the pipeline desc, library and theh shader funtions
    [pipelineDescriptor dealloc];
    [vertShader release];
    [fragShader release];
    [mtlLibrary release];
    
    return true;
}

void ShutdownMetal() {
    NSLog(@"Shutingdown Metal");
    [gDevice release];
    [gCommandQueue release];
    [gRenderPipelineState release];
}

OSStatus SummitSound(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                          UInt32 inNumberFrames, AudioBufferList *ioData) {
    int16 *soundBuffer = (int16 *)ioData->mBuffers[0].mData;
    //memset(soundBuffer, 0, sizeof(int32) * inNumberFrames);
    
// TODO: go through the actived channels in the system and play the if the are playing
    if(gMacSoundSys.channels == nullptr) return noErr;

    MacSoundHandle handle = gMacSoundSys.first;
    while(handle != -1) {
        MacSoundChannel *channel = gMacSoundSys.channels + handle; 
        
        if(channel->playing) {
            int32 samplesLeft = channel->sampleCount - channel->currentSample;
            int32 samplesToStream = MIN(inNumberFrames, samplesLeft);

            int16 *dst = soundBuffer;
            int16 *src = (int16 *)((int32 *)channel->stream->data + channel->currentSample);
            for (UInt32 i = 0; i < samplesToStream; i++)
            {
                *dst++ = *src++;
                *dst++ = *src++;
            }

            if(channel->loop) {
                channel->currentSample = (channel->currentSample + samplesToStream) % channel->sampleCount;
            }
            else {
                channel->currentSample = channel->currentSample + samplesToStream;
                if(channel->currentSample >= channel->sampleCount) {
                    channel->currentSample = 0;
                    channel->playing = false;
                }
            }
        }

        handle = channel->next;
    }

    return noErr;
}


// TODO: Wavefiles  Detour ...
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//this struct is the minimal required header data for a wav file
struct WaveFileHeader
{
	//the main chunk
	unsigned char m_szChunkID[4];
	uint32 m_nChunkSize;
	unsigned char m_szFormat[4];

	//sub chunk 1 "fmt "
	unsigned char m_szSubChunk1ID[4];
	uint32 m_nSubChunk1Size;
	uint16 m_nAudioFormat;
	uint16 m_nNumChannels;
	uint32 m_nSampleRate;
	uint32 m_nByteRate;
	uint16 m_nBlockAlign;
	uint16 m_nBitsPerSample;

	//sub chunk 2 "data"
	unsigned char m_szSubChunk2ID[4];
	uint32 m_nSubChunk2Size;

	//then comes the data!
};

bool WriteWaveFile(const char *szFileName, void *pData, int32 nDataSize, int16 nNumChannels, int32 nSampleRate, int32 nBitsPerSample)
{
	//open the file if we can
	FILE *File = fopen(szFileName,"w+b");
	if(!File)
	{
		return false;
	}

	WaveFileHeader waveHeader;

	//fill out the main chunk
	memcpy(waveHeader.m_szChunkID,"RIFF",4);
	waveHeader.m_nChunkSize = nDataSize + 36;
	memcpy(waveHeader.m_szFormat,"WAVE",4);

	//fill out sub chunk 1 "fmt "
	memcpy(waveHeader.m_szSubChunk1ID,"fmt ",4);
	waveHeader.m_nSubChunk1Size = 16;
	waveHeader.m_nAudioFormat = 1;
	waveHeader.m_nNumChannels = nNumChannels;
	waveHeader.m_nSampleRate = nSampleRate;
	waveHeader.m_nByteRate = nSampleRate * nNumChannels * nBitsPerSample / 8;
	waveHeader.m_nBlockAlign = nNumChannels * nBitsPerSample / 8;
	waveHeader.m_nBitsPerSample = nBitsPerSample;

	//fill out sub chunk 2 "data"
	memcpy(waveHeader.m_szSubChunk2ID,"data",4);
	waveHeader.m_nSubChunk2Size = nDataSize;

	//write the header
	fwrite(&waveHeader,sizeof(WaveFileHeader),1,File);

	//write the wave data itself
	fwrite(pData,nDataSize,1,File);

	//close the file and return success
	fclose(File);
	return true;
}

MacSoundStream LoadWavFile(const char *szFileName) {

    FILE *file = fopen(szFileName, "rb");
    if(!file) {
        MacSoundStream zero = {};
        return zero;
    }
    // go to the end of the file
    fseek(file, 0, SEEK_END);
    // get the size of the file to alloc the memory we need
    long int fileSize = ftell(file);
    // go back to the start of the file
    fseek(file, 0, SEEK_SET);
    // alloc the memory
    uint8 *wavData = (uint8 *)malloc(fileSize + 1);
    memset(wavData, 0, fileSize + 1);
    // store the content of the file
    fread(wavData, fileSize, 1, file);
    wavData[fileSize] = '\0'; // null terminating string...
    fclose(file);

   
    WaveFileHeader *header = (WaveFileHeader *)wavData;
    void *data = (wavData + sizeof(WaveFileHeader)); 

    MacSoundStream stream;
    stream.data = data;
    stream.size = header->m_nSubChunk2Size;
    return stream;
}


MacSoundStream Create4SecSoundStream() {
    // Fill this buffer with four seconds of sound
    int32 nSampleRate = 44100;
    int32 nNumSeconds = 4;
    int32 nNumChannels = 2;

    int32 nNumSamples = nSampleRate * nNumChannels * nNumSeconds;
    const double amplitude = 10000.0;
    static double theta = 0.0;
    double theta_increment = 2.0 * M_PI * frequency / 44100;

    int32 *soundBuffer = (int32 *)malloc(nNumSamples * sizeof(int32));

    // Generate the samples
    for(int32 nIndex = 0; nIndex < nNumSamples; nIndex++)
    {
        unsigned short maxShort = -1;
        short value = (short)(sin(theta) * amplitude);

        int16 *dst = (int16 *)&soundBuffer[nIndex];
        dst[0] = value;
        dst[1] = value;

        theta += theta_increment;
        if (theta > 2.0 * M_PI)
        {
            theta -= 2.0 * M_PI;
        }
    }

    MacSoundStream stream;
    
    stream.data = (void *)soundBuffer;
    stream.size = nNumSamples * sizeof(int32);

    return stream;

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
    NSLog(@"Handmade Mac is running!");

    NSRect windowRect =  NSMakeRect(0.0f, 0.0f, 800.0f, 600.0f);

    GameWindow *window = [[GameWindow alloc] initWithContentRect: windowRect
                                                       styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                         backing: NSBackingStoreBuffered
                                                           defer: NO];

    WindowDelegate *windowDelegate = [[WindowDelegate alloc] init];
    [window setDelegate: windowDelegate];

    [window setBackgroundColor: [NSColor whiteColor]];
    [window setTitle: @"HandmadeMac"];
    [window makeKeyAndOrderFront: nil];

    if(InilializeMetal() == false) {
        NSLog(@"ERROR: Metal Fail Initialization!");
        return 1;
    }
    
    MTKView *mtkView = [[MTKView alloc] initWithFrame: windowRect
                                               device: gDevice];
    window.contentView = mtkView;

    
    MetalViewDelegate *viewDelegate = [[MetalViewDelegate alloc] init];
    [mtkView setDelegate: viewDelegate];

    InilializeSoftwareRenderer();
    
    GameInputController *gameInputController = [[GameInputController alloc] init];
    [[NSNotificationCenter defaultCenter]addObserver:gameInputController selector:@selector(controlWasConnected:) name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:gameInputController selector:@selector(controlWasDisconnected:) name:GCControllerDidDisconnectNotification object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:gameInputController selector:@selector(keyboardWasConnected:) name:GCKeyboardDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:gameInputController selector:@selector(keyboardWasDisconnected:) name:GCKeyboardDidDisconnectNotification object:nil];
    
    gCurrentInput = &gInputs[0];
    gLastInput = &gInputs[1];
    
    
    
    // TODO: Initialize Audio
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AudioUnit toneUnit;
    // Configure the search parameters to find the default playback output unit
    // (called the kAudioUnitSubType_RemoteIO on iOS but
    // kAudioUnitSubType_DefaultOutput on Mac OS X)
    AudioComponentDescription defaultOutputDescription;
    defaultOutputDescription.componentType = kAudioUnitType_Output;
    defaultOutputDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
    defaultOutputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    defaultOutputDescription.componentFlags = 0;
    defaultOutputDescription.componentFlagsMask = 0;
    
    // Get the default playback output unit
    AudioComponent defaultOutput = AudioComponentFindNext(NULL, &defaultOutputDescription);
    // Create the toneUnit
    OSErr err = AudioComponentInstanceNew(defaultOutput, &toneUnit);
    
    // init render callback struct for core audio
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = SummitSound;
    // TODO: pass the game audio buffer ...
    
    err = AudioUnitSetProperty(toneUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
        0, &callbackStruct, sizeof(callbackStruct));
    
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = 44100;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsPacked | kAudioFormatFlagIsSignedInteger; 
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = 2 * sizeof(short);
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mBitsPerChannel =  sizeof(short) * 8;
    streamFormat.mBytesPerFrame = 2 * sizeof(short);
    
    err = AudioUnitSetProperty (toneUnit,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        0,
        &streamFormat,
        sizeof(AudioStreamBasicDescription));


    MacSoundSysInitialize(1024);


    // Load the song wav file in to the sound system
    // TODO: stream this file instead of load the entire file
    NSString *testSoundPath = [[NSBundle mainBundle] pathForResource: @"test" ofType: @"wav"];
    MacSoundStream testSound = LoadWavFile([testSoundPath UTF8String]);
    MacSoundHandle testSoundHandle = MacSoundSysAdd(&testSound, true, true);
    
    // Initialize the audio unit
    err = AudioUnitInitialize(toneUnit);
    // Start playback
    err = AudioOutputUnitStart(toneUnit);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return NSApplicationMain(argc, argv);
}
