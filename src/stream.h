#ifndef STREAM_H
#define STREAM_H

#include "dsock.h"

int extract_sock_msg(SOCKET s, char* buf, unsigned long long buf_size, unsigned long long* msg_size);

int send_sock_msg(SOCKET s, char* buf, unsigned long long msg_size);

#endif
