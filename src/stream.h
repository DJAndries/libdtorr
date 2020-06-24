#ifndef STREAM_H
#define STREAM_H

#include "dtorr/structs.h"
#include "dsock.h"

struct unsent_info {
  char* buf;
  unsigned long long offset;
  unsigned long long length;
};
typedef struct unsent_info unsent_info;

int extract_sock_msg(SOCKET s, char* buf, unsigned long long buf_size, unsigned long long* msg_size);

int send_sock_msg(dtorr_peer* peer, char* buf, unsigned long long msg_size);

int attempt_resend(dtorr_peer* peer);

void clean_unsent_info(dtorr_peer* peer);

#endif
