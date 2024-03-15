//
//  mac_handmade.mm
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


#include <mach/mach_init.h>
#include <mach/mach_time.h>

#include "common.h"
#include "algebra.h"
#include "memory.h"
#include "network.h"
#include "client.h"

#include "mac_renderer.cpp"
#include "mac_input.cpp"
#include "mac_sound_sys.cpp"

#include "memory.cpp"
#include "network.cpp"
#include "wave_file.cpp"
#include "collision.cpp"
#include "tilemap.cpp"
#include "draw.cpp"
#include "entity.cpp"
#include "client.cpp"

@interface MacWindow : NSWindow
@end

@interface MacWindowDelegate: NSObject<NSWindowDelegate>
@end

@interface MacViewDelegate: NSObject<MTKViewDelegate>
@end

@interface MacInputController: NSObject
- (void)controlWasConnected:(NSNotification *)notification;
- (void)controlWasDisconnected:(NSNotification *)notification;
- (void)keyboardWasConnected:(NSNotification *)notification;
- (void)keyboardWasDisconnected:(NSNotification *)notification;
@end

struct MacApp {
    MacWindow *window;
    MacWindowDelegate *windowDelegate;
    MTKView *view;
    MacViewDelegate *viewDelegate;
    MacInputController *inputController;
    AudioUnit audioUnit;

    uint64 lastTimeCounter;
    uint64 frequency;
};

void MacApplicationShutdown(MacApp *app);

// globals
static Memory gMemory;
static MacApp gMacApp;
// main systems
static MacRenderer gRenderer;
static MacInput gInput;
static MacSoundSystem gMacSoundSys;

static const int32 WINDOW_WIDTH  = 800; 
static const int32 WINDOW_HEIGHT = 600; 

static const int32 GAME_MEMORY_SIZE = MB(100);

@implementation MacWindow
- (void)keyDown:(NSEvent *)event { /* NOTE: this removes the beeps that the keyboard does */ }
@end

@implementation MacWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    // Free all memory that we allocated
    static bool appShutdown = false;
    if(appShutdown == false) {
        appShutdown = true;
        MacApplicationShutdown(&gMacApp);
    }
    [NSApp terminate: nil];
}
@end

@implementation MacViewDelegate
// NOTE: game's render loop
- (void)drawInMTKView:(MTKView *) view {

    uint64 currentTimeCounter = mach_absolute_time();
    uint64 elapsedTime = (currentTimeCounter - gMacApp.lastTimeCounter) * gMacApp.frequency;
    float64 deltaTime = (float64)elapsedTime * 1.0E-9;
    gMacApp.lastTimeCounter = currentTimeCounter;

    MacProcessInput(&gInput, gInput.currentInput);
    gInput.currentInput->deltaTime = (float32)deltaTime;
    
    GameBackBuffer gameBackBuffer;
    gameBackBuffer.data = (void *)gRenderer.backBuffer;
    gameBackBuffer.width = gRenderer.textureDesc.width;
    gameBackBuffer.height = gRenderer.textureDesc.height;
    gameBackBuffer.pitch = gRenderer.textureDesc.width * 4;
    GameUpdateAndRender(&gMemory, &gMacSoundSys.sound, gInput.currentInput, &gameBackBuffer);

    DrawSoftwareRenderer(&gRenderer, view);
    
    GameInput *tmp = gInput.currentInput;
    gInput.currentInput = gInput.lastInput;
    gInput.lastInput = tmp;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize) size {
}
@end

@implementation MacInputController
- (void)controlWasConnected:(NSNotification *)notification {
    // a controller was connected
    GCController *controller = (GCController *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nController connected\nName: %@\n", controller.vendorName];
    NSLog(@"%@", status);
    for(uint32 i = 0; i < MAC_MAX_CONTROLLER_COUNT; i++) {
        if(gInput.controllers[i] == nil) {
            gInput.controllers[i] = controller;
            gInput.controllerConected++;
            break;
        }
    }
}

- (void)controlWasDisconnected:(NSNotification *)notification {
    // a controller was disconnected
    GCController *controller = (GCController *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nController disconnected:\n%@", controller.vendorName];
    NSLog(@"%@", status);
    
    for(uint32 i = 0; i < MAC_MAX_CONTROLLER_COUNT; i++) {
        if(gInput.controllers[i] == controller) {
            gInput.controllers[i] = nil;
            gInput.controllerConected--;
            break;
        }
    }
}

- (void)keyboardWasConnected:(NSNotification *)notification {
    GCKeyboard *keyboard = (GCKeyboard *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nKeyboard connected\nName: %@\n", keyboard.vendorName];
    NSLog(@"%@", status);
    gInput.keyboard = keyboard;
}
- (void)keyboardWasDisconnected:(NSNotification *)notification {
    GCKeyboard *keyboard = (GCKeyboard *)notification.object;
    NSString *status = [NSString stringWithFormat:@"\nKeyboard disconnected:\n%@", keyboard.vendorName];
    NSLog(@"%@", status);
    gInput.keyboard = nil;
}

@end

const char *MacGetPath(const char *name, const char *ext) {
    NSString *path = [[NSBundle mainBundle] pathForResource: [NSString stringWithUTF8String:name] 
                                                     ofType: [NSString stringWithUTF8String:ext]];
    return [path UTF8String];
}

Sound MacGameSoundLoad(Arena *arena, const char *name, bool playing, bool loop) {
    NSString *soundPath = [[NSBundle mainBundle] pathForResource: [NSString stringWithUTF8String:name] 
                                                          ofType: @"wav"];
    if(soundPath != nil) {
        // TODO: pass our memory to store this files
        MacSoundStream stream = LoadWavFile(arena, [soundPath UTF8String]);
        MacSoundHandle soundHandle = MacSoundSysAdd(&gMacSoundSys, stream, playing, loop);
        
        Sound sound;
        sound.data = stream.data;
        sound.size = stream.size;
        sound.handle = (SoundHandle)soundHandle;
        return sound;
    }

    Sound null;
    null.data = nullptr;
    null.size = 0;
    null.handle = -1;
    return null;


}

void MacGameSoundRemove(Sound *sound) {
    MacSoundSysRemove(&gMacSoundSys, (MacSoundHandle *)&sound->handle);
    sound->handle = -1;
}

void MacGameSoundPlay(Sound sound) {
    MacSoundSysPlay(&gMacSoundSys, (MacSoundHandle)sound.handle);
}

void MacGameSoundPause(Sound sound) {
    MacSoundSysPause(&gMacSoundSys, (MacSoundHandle)sound.handle);
}

void MacGameSoundRestart(Sound sound) {
    MacSoundSysRestart(&gMacSoundSys, (MacSoundHandle)sound.handle);
}

void MacAppicationInitialize(MacApp *app) {

    NSRect windowRect =  NSMakeRect(0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    app->window = [[MacWindow alloc] initWithContentRect: windowRect
                                               styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                 backing: NSBackingStoreBuffered
                                                   defer: NO];

    app->windowDelegate = [[MacWindowDelegate alloc] init];
    [app->window setDelegate: app->windowDelegate];
    [app->window setBackgroundColor: [NSColor whiteColor]];
    [app->window setTitle: @"MacClient"];
    [app->window makeKeyAndOrderFront: nil];

    if(InilializeMetal(&gRenderer) == false) {
        NSLog(@"ERROR: Metal Fail Initialization!");
        return;
    }

    app->view = [[MTKView alloc] initWithFrame: windowRect
                                        device: gRenderer.device];
    app->window.contentView = app->view;

    
    app->viewDelegate = [[MacViewDelegate alloc] init];
    [app->view setDelegate: app->viewDelegate];


    InilializeSoftwareRenderer(&gRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    app->inputController = [[MacInputController alloc] init];
    [[NSNotificationCenter defaultCenter]addObserver:app->inputController selector:@selector(controlWasConnected:) name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:app->inputController selector:@selector(controlWasDisconnected:) name:GCControllerDidDisconnectNotification object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:app->inputController selector:@selector(keyboardWasConnected:) name:GCKeyboardDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:app->inputController selector:@selector(keyboardWasDisconnected:) name:GCKeyboardDidDisconnectNotification object:nil];
    
    gInput.currentInput = &gInput.inputs[0];
    gInput.lastInput = &gInput.inputs[1];
 
    // alloc all the memory the game is going to use
    gMemory.size = GAME_MEMORY_SIZE;
    gMemory.used = 0;
    gMemory.data = (uint8 *)mmap(0, gMemory.size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0); 

    InitializeCoreAudio(&gMacSoundSys, &app->audioUnit);

    MacSoundSysInitialize(&gMacSoundSys, 1024);

    // Initialize GameSound callbacks
    gMacSoundSys.sound.Load = MacGameSoundLoad;
    gMacSoundSys.sound.Remove = MacGameSoundRemove;
    gMacSoundSys.sound.Play = MacGameSoundPlay;
    gMacSoundSys.sound.Pause = MacGameSoundPause;
    gMacSoundSys.sound.Restart = MacGameSoundRestart;

    // Initialize the audio unit
    AudioUnitInitialize(app->audioUnit);
    // Start playback
    AudioOutputUnitStart(app->audioUnit);

}

void MacApplicationShutdown(MacApp *app) {
    MacSoundSysShutdown(&gMacSoundSys);
    ShutdownCoreAudio(&app->audioUnit);
    munmap((void *)gMemory.data, gMemory.size);
    [app->inputController dealloc];
    ShutdownSoftwareRenderer(&gRenderer);
    [app->viewDelegate dealloc];
    ShutdownMetal(&gRenderer);
    [app->windowDelegate dealloc];
}



int main(int argc, const char *argv[]) {
    NSLog(@"Handmade Mac is running!");
    MacAppicationInitialize(&gMacApp);

    gInput.currentInput->GetPath = MacGetPath;

    GameInitialize(&gMemory, &gMacSoundSys.sound, gInput.currentInput);
    
    mach_timebase_info_data_t timeBaseInfo;
    mach_timebase_info(&timeBaseInfo);

    
    gMacApp.lastTimeCounter = mach_absolute_time();
    gMacApp.frequency = timeBaseInfo.numer / timeBaseInfo.denom;


    return NSApplicationMain(argc, argv);
}
