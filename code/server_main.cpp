#include <stdio.h>
#include <iostream>
#include <memory.h>
#include <chrono>
#include <unistd.h>


#include "common.h"
#include "algebra.h"
#include "memory.h"
#include "network.h"
#include "server.h"

#include "memory.cpp"
#include "network.cpp"
#include "collision.cpp"
#include "tilemap.cpp"
#include "entity.cpp"
#include "server.cpp"

int32 main(int32 argc, char **argv) {

    // TODO: create a utility file for this kind of functions...
    // print current working directory
    const size_t cwdSize = 256;
    char cwd[cwdSize];
    getcwd(cwd, cwdSize);
    printf("cwd: %s\n", cwd);
    
    Memory memory;
    memory.size = MB(100);
    memory.used = 0;
    memory.data = (uint8 *)malloc(memory.size);


    ServerInitialize(&memory);

    auto last = std::chrono::high_resolution_clock::now( );    
    for(;;) {        
        auto current = std::chrono::high_resolution_clock::now( );    
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>( current - last );

        float32 dt = (float32)elapsed.count() / 1000000000.0f;
 
        ServerUpdate(&memory, dt);

        last = current;


    }
    ServerShutdown(&memory);

    free(memory.data);

    return 0;
}
