#import "socket.h"
#include <string.h>

int dsock_init() {
  #ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), data);
  #else
    return 0;
  #endif
}

int dsock_clean() {
  #ifdef _WIN32
    return WSACleanup();
  #else
    return 0;
  #endif
}