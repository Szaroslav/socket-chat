#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "socket.h"

#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>

#define          CONNECTION_SUCCESS    0
#define          CONNECTION_FAILURE    -1
#define          HELLO_MESSAGE         "HELLO"
extern const int HELLO_MESSAGE_LENGTH;
#define          EXIT_MESSAGE          "EXIT"
extern const int EXIT_MESSAGE_LENGTH;

typedef struct connection {
  bool               tcp_active;
  int                tcp_socket_fd;
  bool               udp_active;
  struct sockaddr_in udp_client_address;
  pthread_t          thread;
} connection;

typedef struct connection_params {
  int id;
  int tcp_socket_fd;
} connection_params;

bool js_is_hello(const char *message);
void js_udp_hello(
  int socket_fd,
  int connection_id,
  const sockaddr *destination_address,
  socklen_t address_length);
int  js_connection_id_from_udp_message(const char *message);
void js_init_connection_array(connection *connections, int size);
int  js_tcp_connection(connection *connections, int size, int socket_fd);
void js_udp_connection(
    connection *connections,
    int id,
    const struct sockaddr_in *address
);
int  js_connection_id(connection *connections, int size);
void js_prepare_connection_termination(connection *connections, int id);
void js_terminate_connection(connection *connections, int id);

#endif // __CONNECTION_H__
