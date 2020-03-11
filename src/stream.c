#include "stream.h"
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

int send_sock_msg(SOCKET s, char* buf, unsigned long long msg_size) {
  char length_buf[4];

  uint_to_bigend(length_buf, (unsigned int)msg_size);

  if (send(s, length_buf, 4, 0) != 4) {
    return 1;
  }

  if (send(s, buf, msg_size, 0) != msg_size) {
    return 2;
  }

  return 0;
}
