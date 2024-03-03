#include "connection.h"
#include "logger.h"
#include "thread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define   BUFFER_SIZE            64
const int HELLO_MESSAGE_LENGTH = strlen(HELLO_MESSAGE);
const int EXIT_MESSAGE_LENGTH  = strlen(EXIT_MESSAGE);

static char buffer[BUFFER_SIZE];

bool js_is_hello_message(const char *message) {
  for (int i = 0; i < HELLO_MESSAGE_LENGTH; i++) {
    if (message[i] != HELLO_MESSAGE[i]) {
      return false;
    }
  }
  return true;
}

void js_init_connection_array(connection *connections, int size) {
  memset(connections, 0, size * sizeof(connection));
}

int js_connection(connection *connections, int size, int socket_fd) {
  const int connection_id = js_connection_id(connections, size);
  if (connection_id == CONNECTION_FAILURE) {
    warn("All connections are currently active", WARNING_TITLE);
    return CONNECTION_FAILURE;
  }

  connections[connection_id].socket_fd = socket_fd;
  connections[connection_id].active    = true;
  success("Connection creation succeeded", SUCCESS_TITLE);

  return connection_id;
}

int js_connection_id(connection *connections, int size) {
  for (int i = 0; i < size; i++) {
    if (!connections[i].active) {
      return i;
    }
  }
  return CONNECTION_FAILURE;
}

void js_prepare_connection_termination(connection *connections, int id) {
  if (!connections[id].active) {
    warn("Inactive connection termination", WARNING_TITLE);
    return;
  }

  sprintf(buffer, "Preparing termination of connection %d", id);
  info(buffer, INFO_TITLE);
  write(connections[id].socket_fd, EXIT_MESSAGE, EXIT_MESSAGE_LENGTH + 1);
}

void js_terminate_connection(connection *connections, int id) {
  if (!connections[id].active) {
    warn("Inactive connection termination", WARNING_TITLE);
    return;
  }

  memset(&connections[id], 0, sizeof(connection));
  sprintf(buffer, "Termination of connection %d succeeded", id);
  success(buffer, SUCCESS_TITLE);
  pthread_exit(NULL);
}
