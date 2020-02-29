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

  if ((conn_sock = accept(config->serv_sock, (sockaddr*)&conn_info, 0)) == INVALID_SOCKET) {
    return 0;
  }

  conn_port = ntohs(conn_info.sin_port);
  conn_ip = inet_ntoa(conn_info.sin_addr);

  dlog(config, LOG_LEVEL_INFO, "Accepted connection from %s:%hu", conn_ip, conn_port);

  return peer_recv_handshake(config, conn_sock, conn_ip, conn_port);
}
