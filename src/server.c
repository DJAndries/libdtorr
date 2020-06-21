#include <stdio.h>
#include <string.h>
#include "dtorr/server.h"
#include "dsock.h"
#include "handshake.h"
#include "log.h"
#include "close.h"
#include "msg_out.h"

int peer_server_start(dtorr_config* config) {
  if ((config->serv_sock = dsock_start_server(config->port)) == INVALID_SOCKET) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to start server on port %hu", config->port);
    return 1;
  }

  dlog(config, LOG_LEVEL_INFO, "Started server on port %hu", config->port);
  return 0;
}

int peer_server_accept(dtorr_config* config) {
  SOCKET conn_sock;
  sockaddr_in conn_info;
  unsigned short conn_port;
  char* conn_ip;
  int errno_result;
  socklen_t conn_info_sz = sizeof(conn_info);

  if ((conn_sock = accept(config->serv_sock, (sockaddr*)&conn_info, &conn_info_sz)) == INVALID_SOCKET) {
    errno_result = dsock_errno();
    if (errno_result != DEAGAIN && errno_result != DEWOULDBLOCK) {
      dlog(config, LOG_LEVEL_ERROR, "Error accepting connection: %s", strerror(errno_result));
      return 2;
    }
    return 0;
  }

  conn_port = ntohs(conn_info.sin_port);
  conn_ip = inet_ntoa(conn_info.sin_addr);

  if (dsock_set_sock_nonblocking(conn_sock) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to set socket as nonblocking");
    return 1;
  }

  dlog(config, LOG_LEVEL_INFO, "Accepted connection from %s:%hu", conn_ip, conn_port);

  return peer_recv_handshake(config, conn_sock, conn_ip, conn_port);
}
