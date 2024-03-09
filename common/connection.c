#include "connection.h"
#include "logger.h"
#include "thread.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


#define   BUFFER_SIZE            64
const int HELLO_MESSAGE_LENGTH = strlen(HELLO_MESSAGE);
const int EXIT_MESSAGE_LENGTH  = strlen(EXIT_MESSAGE);

static char buffer[BUFFER_SIZE];

bool js_is_hello(const char *message) {
  for (int i = 0; i < HELLO_MESSAGE_LENGTH; i++) {
    if (message[i] != HELLO_MESSAGE[i]) {
      return false;
    }
  }
  return true;
}

void js_udp_hello(
    int socket_fd,
    int connection_id,
    const sockaddr *destination_address,
    socklen_t address_length
) {
  sprintf(buffer, "%d%s", connection_id, HELLO_MESSAGE);
  int sent_bytes = js_socket_send_to(
    socket_fd,
    buffer,
    strlen(buffer) + 1,
    destination_address,
    address_length,
    MSG_DONTWAIT);
  if (sent_bytes < 0) {
    error("Sending UDP HELLO message failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Sending UDP HELLO message failed", SUCCESS_TITLE);
}

int js_connection_id_from_udp_message(const char *message) {
  char connection_id;
  sscanf(message, "%hhd", &connection_id);
  return connection_id;
}

void js_init_connection_array(connection *connections, int size) {
  memset(connections, 0, size * sizeof(connection));
}

int js_tcp_connection(connection *connections, int size, int tcp_socket_fd) {
  const int connection_id = js_connection_id(connections, size);
  if (connection_id == CONNECTION_FAILURE) {
    warn("All connections are currently active", WARNING_TITLE);
    return CONNECTION_FAILURE;
  }

  connections[connection_id].tcp_socket_fd = tcp_socket_fd;
  connections[connection_id].tcp_active    = true;
  success("TCP connection creation succeeded", SUCCESS_TITLE);

  return connection_id;
}

void js_udp_connection(
    connection *connections,
    int id,
    const struct sockaddr_in *address
) {
  memcpy(
    &connections[id].udp_client_address,
    address,
    sizeof(*address));
  connections[id].udp_active = true;
  success("UDP connection creation succeeded", SUCCESS_TITLE);
}

int js_connection_id(connection *connections, int size) {
  for (int i = 0; i < size; i++) {
    if (!connections[i].tcp_active) {
      return i;
    }
  }
  return CONNECTION_FAILURE;
}

void js_prepare_connection_termination(connection *connections, int id) {
  if (!connections[id].tcp_active) {
    warn("Inactive connection termination", WARNING_TITLE);
    return;
  }

  sprintf(buffer, "Preparing termination of connection %d", id);
  info(buffer, INFO_TITLE);
  js_socket_write(connections[id].tcp_socket_fd, EXIT_MESSAGE, EXIT_MESSAGE_LENGTH + 1);
}

void js_terminate_connection(connection *connections, int id) {
  if (!connections[id].tcp_active) {
    warn("Inactive connection termination", WARNING_TITLE);
    return;
  }

  memset(&connections[id], 0, sizeof(connection));
  sprintf(buffer, "Termination of connection %d succeeded", id);
  success(buffer, SUCCESS_TITLE);
  pthread_exit(NULL);
}
