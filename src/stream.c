#include "stream.h"
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "util.h"

#define TEMP_BUF_SIZE 24 * 1024
#define MAX_UNSENT_MSGS 128

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

static int queue_unsent_data(dtorr_peer* peer, char* buf, unsigned long long msg_size, unsigned long long offset) {
  unsent_info* info;

  if (peer->unsent_data_count >= MAX_UNSENT_MSGS) {
    return -1;
  }
 
  info = (unsent_info*)malloc(sizeof(unsent_info));
  if (info == 0) {
    return -2;
  }

  info->buf = buf;
  info->length = msg_size;
  info->offset = offset >= 0 ? offset : 0;
  
  if (list_insert(&peer->unsent_data, info) != 0) {
    free(info);
    return -2;
  }

  peer->unsent_data_count++;

  return 0;
}

int attempt_resend(dtorr_peer* peer) {
  int errno_result;
  long long send_result;
  unsigned long long resend_length;
  dtorr_listnode *it, *next;
  unsent_info* info;

  if (peer->unsent_data == 0) {
    return 0;
  }

  for (it = peer->unsent_data; it != 0; it = next) {
    next = it->next;
    info = (unsent_info*)it->value;

    /* attempt resend */
    resend_length = info->length - info->offset;
    send_result = send(peer->s, info->buf + info->offset, resend_length, 0);

    if ((info->length - info->offset) == send_result) {
      /* sent the rest of the message. free the buf, and remove from list, continue to next msg */
      free(info->buf);
      list_remove(&peer->unsent_data, info);
      free(info);
      peer->unsent_data_count--;
    } else {
      errno_result = dsock_errno();
      if (errno_result != DEAGAIN && errno_result != DEWOULDBLOCK) {
        /* bad error, return */
        return -1;
      } else {
        /* still more of the message that is yet to be sent */
        info->offset = info->offset + (send_result < 0 ? 0 : send_result);
        return 1;
      }
    }
  }
  return 0;
}

int send_sock_msg(dtorr_peer* peer, char* buf, unsigned long long msg_size) {
  int errno_result;
  long long send_result;
  char* send_buf;

  send_buf = malloc(sizeof(char) * (msg_size + 4));

  if (send_buf == 0) {
    return -2;
  }

  /* prepare buffer with msg length and data */
  uint_to_bigend(send_buf, (unsigned int)msg_size);
  memcpy(send_buf + 4, buf, msg_size);

  /* if there is unsent data queued, then add our message to the queue and return */
  if (peer->unsent_data != 0) {
    if (queue_unsent_data(peer, send_buf, msg_size + 4, 0) != 0) {
      free(send_buf);
      return -3;
    }
    return 1;
  }

  /* attempt send */
  send_result = send(peer->s, send_buf, msg_size + 4, 0);

  if (send_result == (msg_size + 4)) {
    /* sent completely */
    return 0;
  } else {
    errno_result = dsock_errno();
    if (errno_result != DEAGAIN && errno_result != DEWOULDBLOCK) {
      /* error unrelated to tcp buffer being full, bad */
      free(send_buf);
      return -1;
    } else {
      /* queue the rest of the data, if tcp buffer is full */
      if (queue_unsent_data(peer, send_buf, msg_size + 4, send_result) != 0) {
        free(send_buf);
        return -3;
      }
      return 1;
    }
  }
}

void clean_unsent_info(dtorr_peer* peer) {
  dtorr_listnode* it;
  unsent_info* info;
  for (it = peer->unsent_data; it != 0; it = it->next) {
    info = (unsent_info*)it->value;
    free(info->buf);
    free(info);
  }
  if (peer->unsent_data != 0) {
    list_free(peer->unsent_data, 0);
    peer->unsent_data = 0;
  }
}
