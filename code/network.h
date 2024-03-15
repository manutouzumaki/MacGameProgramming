#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

typedef int SOCKET;
const int NO_ERROR = 0;
const int INVALID_SOCKET = -1;
const int WSAECONNRESET = ECONNRESET;
const int WSAEWOULDBLOCK = EAGAIN;
const int SOCKET_ERROR = -1;

struct UDPAddress {
    sockaddr addrs; 
};

struct UDPSocket {
    SOCKET handle;
};

uint32 IP(uint32 a, uint32 b, uint32 c, uint32 d);
UDPAddress UDPAddresCreate(uint32 ip, uint32 port);
UDPSocket UDPSocketCreate();
void UDPSocketDestroy(UDPSocket *socket);
int32 UDPGetLastError();
void UDPSocketBind(UDPSocket *socket, UDPAddress *addrs);
int32 UDPSocketSendTo(UDPSocket *socket, const void *inToSend, int32 inLength, UDPAddress *toAddrs);
int32 UDPSocketReceiveFrom(UDPSocket *socket, void *inToReceive, int32 inMaxLength, UDPAddress *outFromAddrs);
int32 UDPSocketSetNonBlockingMode(UDPSocket *socket, bool inShouldBeNonBlocking );
