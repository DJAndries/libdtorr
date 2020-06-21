#ifndef DTORR_DSOCK_H
#define DTORR_DSOCK_H

#include "uri.h"
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define addrinfo ADDRINFOA
  #define sockaddr SOCKADDR
  #define sockaddr_in SOCKADDR_IN
  #define sockaddr_in6 SOCKADDR_IN6
  #define in_addr IN_ADDR
  #define DEWOULDBLOCK WSAEWOULDBLOCK
  #define DEAGAIN WSAEWOULDBLOCK
  #define DEINPROGRESS WSAEINPROGRESS || WSAEWOULDBLOCK
  #define socklen_t int
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <errno.h>
  #include <unistd.h>
  typedef struct addrinfo addrinfo;
  typedef struct sockaddr_in sockaddr_in;
  typedef struct sockaddr sockaddr;
  typedef struct in_addr in_addr;
  #define DEWOULDBLOCK EWOULDBLOCK
  #define DEAGAIN EAGAIN
  #define DEINPROGRESS EINPROGRESS
  #define SOCKET int
  #define INVALID_SOCKET -1
#endif

int dsock_init();

SOCKET dsock_connect_uri(parsed_uri* uri);

SOCKET dsock_connect(char* host, unsigned short port);

SOCKET dsock_start_server(unsigned short port);

int dsock_set_sock_nonblocking(SOCKET s);

int dsock_recv_timeout(SOCKET s, char* buf, unsigned long long buf_size, unsigned int ms_wait);

SOCKET dsock_close(SOCKET s);

int dsock_clean();

int dsock_errno();

#endif
