#ifndef DSOCK_H
#define DSOCK_H

#ifdef _WIN32
  #include <winsock2.h>
#else
  #include <sys/socket.h>
  #define SOCKET int
  #define INVALID_SOCKET -1
#endif

int dsock_init();

int dsock_clean();

#endif