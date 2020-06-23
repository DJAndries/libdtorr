#include "stream.h"
#include <stdlib.h>
#include <string.h>
#include "util.h"

#define TEMP_BUF_SIZE 24 * 1024

int extract_sock_msg(SOCKET s, char* buf, unsigned long long buf_size, unsigned long long* msg_len) {
  unsigned long long bytes_avail;
  unsigned int len_val;

  char temp_buf[TEMP_BUF_SIZE];

  bytes_avail = recv(s, temp_buf, TEMP_BUF_SIZE, MSG_PEEK);

  if (bytes_avail == -1 || bytes_avail < 4) {
    return 1;
  }

  len_val = bigend_to_uint(temp_buf);
  
  if (len_val > buf_size) {
    return 2;
  }

  if ((bytes_avail - 4) < len_val) {
    return 1;
  }

  recv(s, temp_buf, 4, 0);
  recv(s, buf, len_val, 0);
  *msg_len = len_val;

  return 0;
}

static int handle_send_result(dtorr_peer* peer, char* buf, unsigned long long msg_size, long long send_result) {
  int errno_result;
  if ((msg_size - peer->unsent_data_offset) == send_result) {
    free(buf);
    peer->unsent_data = (char*)(peer->unsent_data_offset = peer->unsent_data_length = 0);
    return 0;
  }

  if (send_result < 0) {
    errno_result = dsock_errno();
    if (errno_result != DEAGAIN && errno_result != DEWOULDBLOCK) {
      free(buf);
      peer->unsent_data = (char*)(peer->unsent_data_offset = peer->unsent_data_length = 0);
      return -1;
    }
  }

  peer->unsent_data = buf;
  peer->unsent_data_offset = peer->unsent_data_offset + (send_result < 0 ? 0 : send_result);
  peer->unsent_data_length = msg_size;
  return 1;
}

int attempt_resend(dtorr_peer* peer) {
  long long send_result;
  unsigned long long resend_length;
  if (peer->unsent_data == 0) {
    return 0;
  }

  resend_length = peer->unsent_data_length - peer->unsent_data_offset;
  send_result = send(peer->s, peer->unsent_data + peer->unsent_data_offset, resend_length, 0);

  return handle_send_result(peer, peer->unsent_data, peer->unsent_data_length, send_result);
}

int send_sock_msg(dtorr_peer* peer, char* buf, unsigned long long msg_size) {
  long long send_result;
  char* send_buf = malloc(sizeof(char) * (msg_size + 4));

  if (send_buf == 0) {
    return -2;
  }

  uint_to_bigend(send_buf, (unsigned int)msg_size);
  memcpy(send_buf + 4, buf, msg_size);

  send_result = send(peer->s, send_buf, msg_size + 4, 0);

  return handle_send_result(peer, send_buf, msg_size + 4, send_result);
}
