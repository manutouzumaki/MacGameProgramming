//
//  mac_sound_sys.cpp
//
//
//  Created by Manuel Cabrerizo on 16/02/2024.
//

typedef int32 MacSoundHandle; 

struct MacSoundStream {
    void *data;
    size_t size;
};

struct MacSoundChannel {
    MacSoundStream stream;

    int32 sampleCount;
    int32 currentSample;

    int32 next;
    int32 prev;

    bool loop;
    bool playing;
};

struct MacSoundSystem {
    MacSoundChannel *channels;
    size_t channelBufferSize;
    int32 first;
    int32 firstFree;
    int32 channelsCount;
    int32 channelsUsed;

    GameSound sound;
};

void MacSoundSysInitialize(MacSoundSystem *soundSys, int32 maxChannels) {
    soundSys->channelsCount = maxChannels;
    soundSys->channelsUsed = 0;
    soundSys->channelBufferSize = maxChannels * sizeof(MacSoundChannel);
    
    soundSys->channels = (MacSoundChannel *)mmap(0, soundSys->channelBufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    memset(soundSys->channels, 0, soundSys->channelBufferSize);
    
    soundSys->first = -1;
    soundSys->firstFree = 0;
    
    // Initialize the channels and free list
    for(int32 i = 0; i < maxChannels; i++) {
        MacSoundChannel *channel = soundSys->channels + i; 
        channel->stream.data = nullptr;
        channel->stream.size = 0;
        channel->loop = false;
        channel->playing = false;
        if(i < (maxChannels - 1))
            channel->next = i + 1;
        else
            channel->next = -1;

        if(i == 0)
            channel->prev = -1;
        else
            channel->prev = i - 1;
    }
}

void MacSoundSysShutdown(MacSoundSystem *soundSys) {
    if(soundSys->channels) {
        munmap(soundSys->channels, soundSys->channelBufferSize);
        soundSys->channels = nullptr;
        soundSys->channelsUsed = 0;
    }
}

MacSoundHandle MacSoundSysAdd(MacSoundSystem *soundSys, MacSoundStream stream, bool playing, bool looping) {

    if((soundSys->channelsUsed + 1) > soundSys->channelsCount) {
        NSLog(@"Sound system full!!!");
        return -1;
    }

    // find a free channel
    MacSoundHandle handle = soundSys->firstFree;
    if(handle < 0 || handle >= soundSys->channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return -1;
    }

    MacSoundChannel *channel = soundSys->channels + handle; 
    // update the free list
    soundSys->firstFree = channel->next;

    // initialize tha channel
    channel->stream = stream;
    channel->loop = looping;
    channel->playing = playing;
    channel->sampleCount = stream.size / sizeof(int32);
    channel->currentSample = 0;

    channel->next = soundSys->first;
    channel->prev = -1;
 
    // set it as the first element of the active channel list
    soundSys->first = handle;

    soundSys->channelsUsed++;

    // update the next channel prev to the incoming channel
    if(channel->next >= 0) {
        MacSoundChannel *nextChannel = soundSys->channels + channel->next;
        nextChannel->prev = handle;
    }
    
    return handle;
}

void MacSoundSysRemove(MacSoundSystem *soundSys, MacSoundHandle *outHandle) {
    MacSoundHandle handle = *outHandle;
    if(handle < 0 || handle >= soundSys->channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }
  
    // get the channel to remove
    MacSoundChannel *channel = soundSys->channels + handle;

    // remove this channel from the active list
    MacSoundChannel *prevChannel = soundSys->channels + channel->prev;
    MacSoundChannel *nextChannel = soundSys->channels + channel->next;
    prevChannel->next = channel->next;
    nextChannel->prev = channel->prev;

    // add this channel to the free list
    channel->prev = -1;
    channel->next = soundSys->firstFree;
    soundSys->firstFree = handle;
 
    soundSys->channelsUsed--;

    *outHandle = -1;

}

void MacSoundSysPlay(MacSoundSystem *soundSys, MacSoundHandle handle) {
    if(handle < 0 || handle >= soundSys->channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }

    MacSoundChannel *channel = soundSys->channels + handle;
    channel->playing = true;

}

void MacSoundSysPause(MacSoundSystem *soundSys, MacSoundHandle handle) {
    if(handle < 0 || handle >= soundSys->channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }

    MacSoundChannel *channel = soundSys->channels + handle;
    channel->playing = false;

}

void MacSoundSysRestart(MacSoundSystem *soundSys, MacSoundHandle handle) {
    if(handle < 0 || handle >= soundSys->channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }

    MacSoundChannel *channel = soundSys->channels + handle;
    channel->playing = false;
    channel->currentSample = 0;
}

// CoreAudio Callback
OSStatus CoreAudioCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                          UInt32 inNumberFrames, AudioBufferList *ioData) {

    MacSoundSystem *soundSys = (MacSoundSystem *)inRefCon;

    int16 *soundBuffer = (int16 *)ioData->mBuffers[0].mData;
    memset(soundBuffer, 0, sizeof(int32) * inNumberFrames);
    
    if(soundSys->channels == nullptr) return noErr;

    // go through the actived channels in the system and play the if the are playing
    MacSoundHandle handle = soundSys->first;
    while(handle != -1) {
        MacSoundChannel *channel = soundSys->channels + handle; 
        
        if(channel->playing) {
            int32 samplesLeft = channel->sampleCount - channel->currentSample;
            int32 samplesToStream = MIN(inNumberFrames, samplesLeft);

            int16 *dst = soundBuffer;
            int16 *src = (int16 *)((int32 *)channel->stream.data + channel->currentSample);

            // TODO: simd ...
            for (UInt32 i = 0; i < samplesToStream; i++) {
                int32 oldValue0 = (int32)dst[0];
                int32 oldValue1 = (int32)dst[1];

                int32 newValue0 = (int32)src[0];
                int32 newValue1 = (int32)src[1];

                int32 sum0 = oldValue0 + newValue0;
                int32 sum1 = oldValue1 + newValue1;

                dst[0] = (int16)MAX(MIN(sum0, 32767), -32768);
                dst[1] = (int16)MAX(MIN(sum1, 32767), -32768);

                dst += 2;
                src += 2;
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

void InitializeCoreAudio(MacSoundSystem *soundSys, AudioUnit *audioUnit) {

    AudioComponentDescription defaultOutputDescription;
    defaultOutputDescription.componentType = kAudioUnitType_Output;
    defaultOutputDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
    defaultOutputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    defaultOutputDescription.componentFlags = 0;
    defaultOutputDescription.componentFlagsMask = 0;
    
    // Get the default playback output unit
    AudioComponent defaultOutput = AudioComponentFindNext(NULL, &defaultOutputDescription);
    // Create the toneUnit
    OSErr err = AudioComponentInstanceNew(defaultOutput, audioUnit);
    
    // init render callback struct for core audio
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = CoreAudioCallback;
    callbackStruct.inputProcRefCon = (void *)soundSys;
    
    err = AudioUnitSetProperty(*audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
        0, &callbackStruct, sizeof(callbackStruct));
    
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = 44100;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsPacked | kAudioFormatFlagIsSignedInteger; 
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = 2 * sizeof(int16);
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mBitsPerChannel =  sizeof(int16) * 8;
    streamFormat.mBytesPerFrame = 2 * sizeof(int16);
    
    err = AudioUnitSetProperty (*audioUnit,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        0,
        &streamFormat,
        sizeof(AudioStreamBasicDescription));
}

void ShutdownCoreAudio(AudioUnit *audioUnit) {
    AudioComponentInstanceDispose(*audioUnit);
}
