typedef int32 MacSoundHandle; 

struct MacSoundStream {
    void *data;
    size_t size;
};

struct MacSoundChannel {
    MacSoundStream *stream;

    int32 sampleCount;
    int32 currentSample;

    int32 next;
    int32 prev;

    bool loop;
    bool playing;
};

struct MacSoundSystem {
    MacSoundChannel *channels;
    int32 first;
    int32 firstFree;
    int32 channelsCount;
    int32 channelsUsed;
};

static MacSoundSystem gMacSoundSys;

void MacSoundSysInitialize(int32 maxChannels) {
    gMacSoundSys.channelsCount = maxChannels;
    gMacSoundSys.channelsUsed = 0;
    gMacSoundSys.channels = (MacSoundChannel *)malloc(maxChannels * sizeof(MacSoundChannel));
    
    gMacSoundSys.first = -1;
    gMacSoundSys.firstFree = 0;
    
    // Initialize the channels and free list
    for(int32 i = 0; i < maxChannels; i++) {
        MacSoundChannel *channel = gMacSoundSys.channels + i; 
        channel->stream = nullptr;
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

void MacSoundSysShutdown() {
    if(gMacSoundSys.channels) {
        free(gMacSoundSys.channels);
        gMacSoundSys.channels = nullptr;
        gMacSoundSys.channelsUsed = 0;
    }
}

MacSoundHandle MacSoundSysAdd(MacSoundStream *stream, bool playing, bool looping) {

    if((gMacSoundSys.channelsUsed + 1) > gMacSoundSys.channelsCount) {
        NSLog(@"Sound system full!!!");
        return -1;
    }

    // find a free channel
    MacSoundHandle handle = gMacSoundSys.firstFree;
    if(handle < 0 || handle >= gMacSoundSys.channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return -1;
    }

    MacSoundChannel *channel = gMacSoundSys.channels + handle; 
    // update the free list
    gMacSoundSys.firstFree = channel->next;

    // initialize tha channel
    channel->stream = stream;
    channel->loop = looping;
    channel->playing = playing;
    channel->sampleCount = stream->size / sizeof(int32);
    channel->currentSample = 0;

    channel->next = gMacSoundSys.first;
    channel->prev = -1;
 
    // set it as the first element of the active channel list
    gMacSoundSys.first = handle;

    gMacSoundSys.channelsUsed++;

    // update the next channel prev to the incoming channel
    MacSoundChannel *nextChannel = gMacSoundSys.channels + channel->next;
    nextChannel->prev = handle;
    
    return handle;
}

void MacSoundSysRemove(MacSoundHandle *outHandle) {
    MacSoundHandle handle = *outHandle;
    if(handle < 0 || handle >= gMacSoundSys.channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }
  
    // get the channel to remove
    MacSoundChannel *channel = gMacSoundSys.channels + handle;

    // remove this channel from the active list
    MacSoundChannel *prevChannel = gMacSoundSys.channels + channel->prev;
    MacSoundChannel *nextChannel = gMacSoundSys.channels + channel->next;
    prevChannel->next = channel->next;
    nextChannel->prev = channel->prev;

    // add this channel to the free list
    channel->prev = -1;
    channel->next = gMacSoundSys.firstFree;
    gMacSoundSys.firstFree = handle;
 
    gMacSoundSys.channelsUsed--;

    *outHandle = -1;

}

void MacSoundSysPlay(MacSoundHandle handle) {
    if(handle < 0 || handle >= gMacSoundSys.channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }

    MacSoundChannel *channel = gMacSoundSys.channels + handle;
    channel->playing = true;

}

void MacSoundSysPause(MacSoundHandle handle) {
    if(handle < 0 || handle >= gMacSoundSys.channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }

    MacSoundChannel *channel = gMacSoundSys.channels + handle;
    channel->playing = false;

}

void MacSoundSysRestart(MacSoundHandle handle) {
    if(handle < 0 || handle >= gMacSoundSys.channelsCount) {
        NSLog(@"Invalid Sound Handle!!!");
        return;
    }

    MacSoundChannel *channel = gMacSoundSys.channels + handle;
    channel->playing = false;
    channel->currentSample = 0;
}
