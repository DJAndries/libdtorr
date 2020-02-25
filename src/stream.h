#ifndef STREAM_H
#define STREAM_H

#include "dsock.h"

int extract_sock_msg(SOCKET s, char* buf, unsigned long buf_size, unsigned long* msg_size);

int send_sock_msg(SOCKET s, char* buf, unsigned long msg_size);

#endif
