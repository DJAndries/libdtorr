#ifndef DSOCK_H
#define DSOCK_H

#include "uri.h"
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define addrinfo ADDRINFOA
  #define sockaddr SOCKADDR
  #define sockaddr_in SOCKADDR_IN
  #define sockaddr_in6 SOCKADDR_IN6
  #define in_addr IN_ADDR
  #define EINPROGRESS WSAEINPROGRESS || WSAEWOULDBLOCK
#else
  #include <sys/socket.h>
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

#endif
