uint32 IP(uint32 a, uint32 b, uint32 c, uint32 d) {
    uint32 ip = ( a << 24 ) | ( b << 16 ) | ( c << 8  ) | d;
    return ip;
}

UDPAddress UDPAddresCreate(uint32 ip, uint32 port) {
    UDPAddress result;
    sockaddr_in *inAddress = (sockaddr_in *)&result.addrs;
    inAddress->sin_family = AF_INET;
    inAddress->sin_addr.s_addr = htonl(ip);
    inAddress->sin_port = htons(port);
    return result;
}

UDPSocket UDPSocketCreate() {
    UDPSocket result;
    result.handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(result.handle <= 0) {
        printf("Error creating udp socket\n");
        ASSERT(!"INVALID_CODE_PATH");
    }
    return result;
}

void UDPSocketDestroy(UDPSocket *socket) {
#if _WIN32
	closesocket(socket->handle);
#else
	close(socket->handle);
#endif
}

int32 UDPGetLastError() {
#if _WIN32
	// TODO: ...
#else
	return errno;
#endif
	
}

void UDPSocketBind(UDPSocket *socket, UDPAddress *addrs) {
    if(bind(socket->handle, (const sockaddr *)&addrs->addrs, sizeof(sockaddr)) != 0) {
        printf("Error binding socket\n");
        ASSERT(!"INVALID_CODE_PATH");
    }
}

int32 UDPSocketSendTo(UDPSocket *socket, const void *inToSend, int32 inLength, UDPAddress *toAddrs) {
    int32 byteSentCount = sendto(socket->handle, static_cast<const char *>(inToSend), inLength,
            0, &toAddrs->addrs, sizeof(sockaddr));

    if(byteSentCount <= 0) {
        return -1;
    }
    else {
        return byteSentCount;
    }
}

int32 UDPSocketReceiveFrom(UDPSocket *socket, void *inToReceive, int32 inMaxLength, UDPAddress *outFromAddrs) {
    socklen_t fromLength =  sizeof(sockaddr); 
    int32 readByteCount = recvfrom(socket->handle, static_cast<char *>(inToReceive), inMaxLength,
            0, &outFromAddrs->addrs, &fromLength);
    if(readByteCount >= 0) {
        return readByteCount;
    }
    else {
        int32 error = UDPGetLastError(); 

		if( error == WSAEWOULDBLOCK ) {
			return 0;
		}
		else if( error == WSAECONNRESET ) {
			//this can happen if a client closed and we haven't DC'd yet.
			//this is the ICMP message being sent back saying the port on that computer is closed
			return -WSAECONNRESET;
		}
		else {
			return -error;
		}
    }
}

int32 UDPSocketSetNonBlockingMode(UDPSocket *socket, bool inShouldBeNonBlocking )
{
#if _WIN32
	u_long arg = inShouldBeNonBlocking ? 1 : 0;
	int32 result = ioctlsocket( mSocket, FIONBIO, &arg );
#else
	int32 flags = fcntl( socket->handle, F_GETFL, 0 );
	flags = inShouldBeNonBlocking ? ( flags | O_NONBLOCK ) : ( flags & ~O_NONBLOCK);
	int32 result = fcntl( socket->handle, F_SETFL, flags );
#endif
	
	if( result == SOCKET_ERROR ) {
		return UDPGetLastError();
	}
	else {
		return NO_ERROR;
	}
}
