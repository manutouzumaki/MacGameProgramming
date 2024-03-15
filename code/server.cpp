//
//  handmade.cpp
//
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//

void ServerInitialize(Memory *memory) {
    
    ASSERT((memory->used + sizeof(GameState)) <= memory->size);
    
    GameState *gameState = (GameState *)memory->data;
    memory->used += sizeof(GameState);

    gameState->assetsArena = ArenaCreate(memory, MB(25));
    gameState->clientArena = ArenaCreate(memory, MB(25));
    gameState->packetArena = ArenaCreate(memory, MB(10));

    gameState->entityPool = MemoryPoolCreate(memory, MaxEntityCount, sizeof(Entity));

    Tilemap collision = LoadCSVTilemap(&gameState->assetsArena, "../assets/tilemaps/collision.csv", 16, 16, true);
    gameState->tilesCountX = collision.width;
    gameState->tilesCountY = collision.height;
    gameState->tiles = collision.tiles;

    // initialize the socket
    gameState->socket = UDPSocketCreate();
    gameState->addrs = UDPAddresCreate(IP(127, 0, 0, 1), 35000);

    UDPSocketBind(&gameState->socket, &gameState->addrs);
    UDPSocketSetNonBlockingMode(&gameState->socket, true);

    // initialize the client hashmap
    gameState->clientsMap.Initialize(&gameState->clientArena, 128);
    gameState->clientCount = 0;


    gameState->timePassFromLastInputPacket = 0;
}

void ServerUpdate(Memory *memory, float32 dt) {
    GameState *gameState = (GameState *)memory->data;

    ArenaClear(&gameState->packetArena); 
    gameState->framePackets = (PacketInput *)ArenaPushSize(&gameState->packetArena, gameState->packetArena.size);
    gameState->framePacketCount = 0;

    char buffer[1200];

	UDPAddress fromAddress;
    memset(&fromAddress, 0, sizeof(UDPAddress));
	//keep reading until we don't have anything to read ( or we hit a max number that we'll process per frame )
	int32 receivedPackedCount = 0;
	int32 totalReadByteCount = 0;

	while( receivedPackedCount < MaxPacketPerFrameCount ) {
		int32 readByteCount = UDPSocketReceiveFrom(&gameState->socket, buffer, 1200, &fromAddress);
		if( readByteCount == 0 ) {
			//nothing to read
			break;
		}
		else if( readByteCount == -WSAECONNRESET ) {
			//port closed on other end, so DC this person immediately
            uint32 uid = MurMur2(&fromAddress, sizeof(UDPAddress), 123);
            Client *client = gameState->clientsMap.GetPtr(uid);
            if(client) {
                RemoveEntity(gameState, client->entity);
                gameState->clientsMap.Remove(uid);
                gameState->clientCount--;
            }
		}
		else if( readByteCount > 0 ) {
            // TODO: create a function to handle the input packets
            MemoryStream inStream = MemoryStreamCreate(buffer, 1200);
            int32 header;
            MemoryStreamRead(&inStream, &header, sizeof(int32)); 
            if(header == PacketHeader) {
                int32 type;
                MemoryStreamRead(&inStream, &type, sizeof(int32));  

                if(type == PacketTypeHello) {
                    printf("Hello packet recived\n");
                    uint32 uid = MurMur2(&fromAddress, sizeof(UDPAddress), 123);
                    Client *client = gameState->clientsMap.GetPtr(uid);
                    // check if we already process the hello packet
                    if(client == nullptr) {
                        // we have to add the new client
                        Client newClient;
                        newClient.uid = uid;
                        newClient.address = fromAddress;
                        newClient.entity = CreatePlayer(gameState);
                        newClient.entity->uid = uid;
                        newClient.entity->address = fromAddress;
                        gameState->clientsMap.Add(uid, newClient);
                        gameState->clientCount++;
                        printf("Client Added\n");
                    }
                    // send welcome packet
                    char sendBuffer[1200];
                    UDPAddress toSendAddr = fromAddress;
                    MemoryStream outStream = MemoryStreamCreate(sendBuffer, 1200);
                    MemoryStreamWrite(&outStream, (void *)&PacketHeader, sizeof(int32));
                    MemoryStreamWrite(&outStream, (void *)&PacketTypeWelcome, sizeof(int32));
                    MemoryStreamWrite(&outStream, (void *)&uid, sizeof(uint32));
                    int32 sentBytes = UDPSocketSendTo(&gameState->socket, sendBuffer, 1200, &toSendAddr);
                    if (sentBytes != 1200) {
                        printf("failed to send Welcome packet\n");
                    }
                    else {
                        printf("Welcome Packet send\n");
                    }
                }
                else if(type == PacketTypeState) {
                    // When we recive a state packet we put it in the packet queue to be process later in the frame 
                    uint32 networkID;
                    MemoryStreamRead(&inStream, &networkID, sizeof(uint32));

                    int32 inputSampleCount;
                    MemoryStreamRead(&inStream, &inputSampleCount, sizeof(int32));

                    InputState inputStates[3];
                    MemoryStreamRead(&inStream, &inputStates[0], sizeof(InputState));
                    MemoryStreamRead(&inStream, &inputStates[1], sizeof(InputState));
                    MemoryStreamRead(&inStream, &inputStates[2], sizeof(InputState));

                    PacketInput packet;
                    packet.header = header;
                    packet.type = type;
                    packet.uid = networkID;
                    packet.samplesCount = inputSampleCount;
                    packet.samples[0] = inputStates[0];
                    packet.samples[1] = inputStates[1];
                    packet.samples[2] = inputStates[2];

                    gameState->framePackets[gameState->framePacketCount++] = packet;
                }
            }
            else {
                printf("bad Packet\n");
            }


			++receivedPackedCount;
			totalReadByteCount += readByteCount;
		}
		else {
			//uhoh, error? exit or just keep going?
		}
	}


    // TODO: update the game state of the server
    // TODO: send the new game state to our clients 
    for(int32 i = 0; i < gameState->framePacketCount; ++i) {
        PacketInput *packet = gameState->framePackets + i;

        Client *client = gameState->clientsMap.GetPtr(packet->uid);
        for(int32 j = packet->samplesCount - 1; j >= 0; --j) {
            InputState inputState = packet->samples[j]; 
            client->entity->vel = inputState.vel;
            MoveEntity(gameState, client->entity, inputState.inputX, inputState.inputY, inputState.deltaTime);
        }

    }

    if(gameState->timePassFromLastInputPacket > TimeBetweenInputPackets) {
        // send new state packet
        char sendBuffer[1200];

        MemoryStream outStream = MemoryStreamCreate(sendBuffer, 1200);

        MemoryStreamWrite(&outStream, (void *)&gameState->clientCount, sizeof(int32));

        Entity *entity = gameState->entities;
        while(entity) {

            MemoryStreamWrite(&outStream, (void *)&PacketHeader, sizeof(int32));
            MemoryStreamWrite(&outStream, (void *)&PacketTypeState, sizeof(int32));
            MemoryStreamWrite(&outStream, (void *)&entity->uid, sizeof(uint32));
            MemoryStreamWrite(&outStream, (void *)&entity->pos, sizeof(Vec2));
            MemoryStreamWrite(&outStream, (void *)&entity->vel, sizeof(Vec2));
            
            entity = entity->next;
        }

        Entity *e = gameState->entities;
        while(e) {
            int32 sentBytes = UDPSocketSendTo(&gameState->socket, sendBuffer, 1200, &e->address);
            if (sentBytes != 1200) {
                printf("failed to send State packet\n");
            }
            e = e->next;
        }

        gameState->timePassFromLastInputPacket -= TimeBetweenInputPackets;
    }

    gameState->timePassFromLastInputPacket += dt;

}

void ServerShutdown(Memory *memory) {
    GameState *gameState = (GameState *)memory->data;

    UDPSocketDestroy(&gameState->socket);

}
