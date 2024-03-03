#include "server.h"

#include "../common/logger.h"
#include "../common/signal.h"
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


#define SERVER_TITLE             "Server   "
#define EQUAL_STRINGS            0
#define MAX_CONNECTION_ID_LENGTH 2
#define WHTBG                    "\e[0;47m"
#define RESET                    "\e[0m"

static bool       exited;
static int        socket_fd;
static connection connections[MAX_ONGOING_CONNECTIONS_COUNT];
static char       buffer[BUFSIZ];
static char       message[MAX_MESSAGE_SIZE_BYTES];

void   init                  ();
void   handle_exit           ();
void   handle_sigint         (int signal);
void   handle_connections    ();
void * handle_connection     (void *params);
void   send_message_to_others(const char *message, int connection_id);

int main() {
  init();
  handle_connections();

  return EXIT_SUCCESS;
}

void init() {
  exited = false;
  atexit(handle_exit);
  js_init_sigint(handle_sigint);
  js_init_connection_array(connections, MAX_ONGOING_CONNECTIONS_COUNT);

  socket_fd = js_socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family      = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port        = htons(PORT);

  js_socket_bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
  js_socket_listen(socket_fd, MAX_PENDING_CONNECTIONS_COUNT);
}

void handle_exit() {
  if (exited) {
    return;
  }

  for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
    if (!connections[i].active) {
      continue;
    }
    js_prepare_connection_termination(connections, i);
  }
  for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
    if (!connections[i].active) {
      continue;
    }
    js_thread_join(connections[i].thread, NULL);
  }
  if (socket_fd != SOCKET_UNDEFINED) {
    js_socket_close(&socket_fd);
  }

  exited = true;
}

void handle_sigint(int signal) {
  handle_exit();
}

void handle_connections() {
  while (true) {
    socklen_t client_address_length;
    struct sockaddr_in client_address;

    const int connection_fd = js_socket_accept(
      socket_fd, (struct sockaddr *) &client_address, &client_address_length);

    const int connection_id = js_connection(
      connections, MAX_ONGOING_CONNECTIONS_COUNT, connection_fd);
    if (connection_id == CONNECTION_FAILURE) {
      continue;
    }

    connection_params params = {
      .id        = connection_id,
      .socket_fd = connection_fd,
    };

    js_thread(
      &connections[connection_id].thread,
      NULL,
      handle_connection,
      (void *) &params);
  }
}

void * handle_connection(void *params) {
  connection_params *p    = (connection_params *) params;
  const int connection_id = p->id,
            socket_fd     = p->socket_fd;

  while (true) {
    js_socket_read(socket_fd, message, MAX_MESSAGE_SIZE_BYTES);

    const bool terminate_connection = strcmp(message, EXIT_MESSAGE) == EQUAL_STRINGS
      && connections[connection_id].active;
    if (terminate_connection) {
      js_terminate_connection(connections, connection_id);
      return NULL;
    }

    sprintf(buffer, "Received message: \"%s\"", message);
    info(buffer, SERVER_TITLE);

    send_message_to_others(message, connection_id);
  }

  return NULL;
}

void send_message_to_others(const char *message, int connection_id) {
  for (int i = 0; i < MAX_ONGOING_CONNECTIONS_COUNT; i++) {
    if (!connections[i].active || i == connection_id) {
      continue;
    }

    sprintf(
      buffer,
      "%s %s%2d %s %s",
      WHTBG, CLIENT_TITLE_WITH_ID, i, RESET, message);
    js_socket_write(connections[i].socket_fd, buffer, strlen(buffer) + 1);
  }
}
