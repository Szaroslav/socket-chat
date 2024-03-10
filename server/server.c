#include "server.h"

#include "../common/logger.h"
#include "../common/byte.h"
#include "../common/string.h"
#include "../common/signal.h"
#include "../common/fd.h"
#include "../common/socket.h"
#include "../common/connection.h"
#include "../common/thread.h"
#include "../client/client.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>


#define SERVER_TITLE             "Server   "
#define FILE_DESCRIPTORS_COUNT   2
#define ONLY_TCP_FILE_DESCRIPTOR 1
#define TCP_SOCKET_FD_POOL_ID    0
#define UDP_SOCKET_FD_POOL_ID    1
#define MAX_CONNECTION_ID_LENGTH 2

static bool               exited;
static char               buffer[BUFFER_SIZE_BYTES];
static char               message[MAX_MESSAGE_SIZE_BYTES];
static int                tcp_socket_fd;
static int                udp_socket_fd;
static int                udp_connection_id;
static struct sockaddr_in server_address;
static struct sockaddr_in client_address;
static socklen_t          client_address_length;
static connection         connections[MAX_ONGOING_CONNECTIONS_COUNT];
static struct pollfd      file_descriptor_pool[FILE_DESCRIPTORS_COUNT];

void   init                  ();
void   init_socket           (socket_type type, struct sockaddr_in *address);
void   handle_exit           ();
void   handle_sigint         (int signal);
void   handle_connections    ();
void * handle_tcp_connection (void *params);
void   receive_udp_message   ();
void   send_message_to_others(const char *message, socket_type type, int connection_id);

int main() {
  init();
  handle_connections();

  return EXIT_SUCCESS;
}

void init() {
  tcp_socket_fd = SOCKET_UNDEFINED;
  udp_socket_fd = SOCKET_UNDEFINED;
  exited        = false;

  atexit(handle_exit);
  js_init_sigint(handle_sigint);
  js_init_connection_array(connections, MAX_ONGOING_CONNECTIONS_COUNT);

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family      = AF_INET,
  server_address.sin_addr.s_addr = INADDR_ANY,
  server_address.sin_port        = htons(PORT),

  init_socket(TCP, &server_address);
  init_socket(UDP, &server_address);
}

void init_socket(socket_type type, struct sockaddr_in *address) {
  int *socket_fd      = &tcp_socket_fd,
       socket_pool_id = TCP_SOCKET_FD_POOL_ID;
  if (type == UDP) {
    socket_fd      = &udp_socket_fd;
    socket_pool_id = UDP_SOCKET_FD_POOL_ID;
  }

  *socket_fd = js_socket(AF_INET, type, 0);
  js_socket_bind(
    *socket_fd,
    (sockaddr *) address,
    sizeof(*address));

  if (type == TCP) {
    js_socket_listen(*socket_fd, MAX_PENDING_CONNECTIONS_COUNT);
  }

  js_fd_nonblock(*socket_fd);

  file_descriptor_pool[socket_pool_id].fd     = *socket_fd;
  file_descriptor_pool[socket_pool_id].events = POLLIN;
}

void handle_exit() {
  if (exited) {
    return;
  }

  for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
    if (!connections[i].tcp_active) {
      continue;
    }
    js_prepare_connection_termination(connections, i);
  }
  for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
    if (!connections[i].tcp_active) {
      continue;
    }
    js_thread_join(connections[i].thread, NULL);
  }

  if (tcp_socket_fd != SOCKET_UNDEFINED) {
    js_socket_close(&tcp_socket_fd);
  }
  if (udp_socket_fd != SOCKET_UNDEFINED) {
    js_socket_close(&udp_socket_fd);
  }

  exited = true;
}

void handle_sigint(int signal) {
  handle_exit();
}

void handle_connections() {
  while (true) {
    js_poll(file_descriptor_pool, FILE_DESCRIPTORS_COUNT, INFINITE_TIMEOUT);

    if (file_descriptor_pool[TCP_SOCKET_FD_POOL_ID].revents & POLLIN) {
      const int connection_fd = js_socket_accept(
        tcp_socket_fd, (sockaddr *) &client_address, &client_address_length);

      const int connection_id = js_tcp_connection(
        connections, MAX_ONGOING_CONNECTIONS_COUNT, connection_fd);
      if (connection_id == CONNECTION_FAILURE) {
        continue;
      }

      connection_params params = {
        .id            = connection_id,
        .tcp_socket_fd = connection_fd,
      };

      js_thread(
        &connections[connection_id].thread,
        NULL,
        handle_tcp_connection,
        (void *) &params);
    }
    if (file_descriptor_pool[UDP_SOCKET_FD_POOL_ID].revents & POLLIN) {
      receive_udp_message();
      if (js_is_hello(message)) {
        info("Received UDP HELLO message", INFO_TITLE);
        js_udp_connection(connections, udp_connection_id, &client_address);
        continue;
      }
      send_message_to_others(message, UDP, udp_connection_id);
    }
  }
}

void * handle_tcp_connection(void *params) {
  connection_params *p    = (connection_params *) params;
  const int connection_id = p->id,
            tcp_socket_fd = p->tcp_socket_fd;

  struct pollfd tcp_socket_poll;
  tcp_socket_poll.fd     = tcp_socket_fd;
  tcp_socket_poll.events = POLLIN;

  int_to_bytes((byte *) message, connection_id);
  js_socket_write(tcp_socket_fd, message, MAX_MESSAGE_SIZE_BYTES);

  while (true) {
    js_poll(&tcp_socket_poll, 1, INFINITE_TIMEOUT);
    if (!(tcp_socket_poll.revents & POLLIN)) {
      continue;
    }

    js_socket_read(tcp_socket_fd, message, MAX_MESSAGE_SIZE_BYTES);
    info(message, INFO_TITLE);

    const bool terminate_connection = js_str_equal(message, EXIT_MESSAGE)
      && connections[connection_id].tcp_active;
    if (terminate_connection) {
      js_terminate_connection(connections, connection_id);
      return NULL;
    }

    sprintf(buffer, "Received message: \"%s\"", message);
    info(buffer, INFO_TITLE);

    send_message_to_others(message, TCP, connection_id);
  }

  return NULL;
}

void receive_udp_message() {
  js_socket_receive_from(
    udp_socket_fd,
    buffer,
    MAX_MESSAGE_SIZE_BYTES,
    (sockaddr *) &client_address,
    &client_address_length,
    MSG_WAITALL);
  udp_connection_id = js_connection_id_from_udp_message(buffer);
  strcpy(message, buffer + UDP_HEADER_SIZE_BYTES);
}

void send_message_to_others(const char *message, socket_type type, int connection_id) {
  info("Sending message to others", INFO_TITLE);

  if (type == TCP) {
    for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
      if (!connections[i].tcp_active || i == connection_id) {
        continue;
      }

      sprintf(
        buffer,
        "%s %s%2d %s %s",
        WHTBG, CLIENT_TITLE_WITH_ID, connection_id, RESET, message);
      js_socket_write(connections[i].tcp_socket_fd, buffer, strlen(buffer) + 1);
    }
  }
  else if (type == UDP) {
    const int message_length = strlen(message);
    int_to_bytes((byte *) buffer, connection_id);

    for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
      if (!connections[i].udp_active || i == connection_id) {
        continue;
      }

      memcpy(buffer + UDP_HEADER_SIZE_BYTES, message, message_length);
      const sockaddr *client_address
        = (const sockaddr *) &connections[i].udp_client_address;
      js_socket_send_to(
        udp_socket_fd,
        buffer,
        UDP_HEADER_SIZE_BYTES + message_length + 1,
        client_address,
        sizeof(*client_address),
        MSG_DONTWAIT);
    }
  }
}
