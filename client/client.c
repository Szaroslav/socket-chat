#include "client.h"

#include "../common/logger.h"
#include "../common/signal.h"
#include "../common/fd.h"
#include "../common/socket.h"
#include "../common/connection.h"
#include "../server/server.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>


#define SENDING_FAILURE        -1
#define EQUAL_STRINGS          0
#define FILE_DESCRIPTORS_COUNT 2
#define STDIN_FD_POOL_ID       0
#define SOCKET_FD_POOL_ID      1
#define POLL_INFINITE_TIMEOUT  -1

static bool exited;
static char buffer[BUFSIZ];
static char message[MAX_MESSAGE_SIZE_BYTES];
static int  socket_fd;
static struct pollfd file_descriptor_pool[FILE_DESCRIPTORS_COUNT];

void   init                  ();
void   handle_exit           ();
void   handle_sigint         (int signal);
char * get_message_from_stdin();
void   send_message          (const char *message);

int main() {
  init();

  while (true) {
    js_poll(file_descriptor_pool, FILE_DESCRIPTORS_COUNT, INFINITE_TIMEOUT);

    if (file_descriptor_pool[STDIN_FD_POOL_ID].revents & POLLIN) {
      const char *message = get_message_from_stdin();
      send_message(message);
    }
    if (file_descriptor_pool[SOCKET_FD_POOL_ID].revents & POLLIN) {
      js_socket_read(socket_fd, buffer, BUFSIZ);
      if (strcmp(buffer, EXIT_MESSAGE) == EQUAL_STRINGS) {
        printf(SIGINT_SYMBOL);
        raise(SIGINT);
      }
      printf("%s\n", buffer);
    }
  }

  return EXIT_SUCCESS;
}

void init() {
  exited    = false;

  atexit(handle_exit);
  js_init_sigint(handle_sigint);
  js_fd_nonblock(STDIN_FILENO);

  socket_fd = js_socket(AF_INET, SOCK_STREAM, 0);

  file_descriptor_pool[STDIN_FD_POOL_ID ].fd     = STDIN_FILENO;
  file_descriptor_pool[STDIN_FD_POOL_ID ].events = POLLIN;
  file_descriptor_pool[SOCKET_FD_POOL_ID].fd     = socket_fd;
  file_descriptor_pool[SOCKET_FD_POOL_ID].events = POLLIN;

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family      = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port        = htons(PORT);

  js_socket_connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
  js_fd_nonblock(socket_fd);
}

void handle_exit() {
  if (exited) {
    return;
  }

  if (socket_fd != SOCKET_UNDEFINED) {
    js_socket_write(socket_fd, EXIT_MESSAGE, EXIT_MESSAGE_LENGTH + 1);
    js_socket_close(&socket_fd);
  }
  exited = true;
}

void handle_sigint(int signal) {
  handle_exit();
}

char * get_message_from_stdin() {
  fgets(message, MAX_MESSAGE_SIZE_BYTES, stdin);
  message[strlen(message) - 1] = '\0';
  return message;
}

void send_message(const char *message) {
  sprintf(buffer, "Sending message: \"%s\"", message);
  info(buffer, CLIENT_TITLE);
  js_socket_write(socket_fd, message, strlen(message) + 1);
}
