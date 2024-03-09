#include "client.h"

#include "../common/logger.h"
#include "../common/string.h"
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
#define FILE_DESCRIPTORS_COUNT 3
#define STDIN_FD_POOL_ID       0
#define TCP_SOCKET_FD_POOL_ID  1
#define UDP_SOCKET_FD_POOL_ID  2
#define ONLY_STDIN_FD          1
#define UDP_MODE               "U"

static bool               exited;
static char               buffer[BUFSIZ];
static char               message[MAX_MESSAGE_SIZE_BYTES];
static int                tcp_socket_fd;
static int                udp_socket_fd;
static int                connection_id;
static struct sockaddr_in server_address;
static socklen_t          server_address_length;
static struct pollfd      file_descriptor_pool[FILE_DESCRIPTORS_COUNT];

void   init                  ();
void   handle_exit           ();
void   handle_sigint         (int signal);
char * input                 (int mode);
void   send_tcp_message      (const char *message);
void   send_udp_message      (const char *message);

int main() {
  init();

  while (true) {
    js_poll(file_descriptor_pool, FILE_DESCRIPTORS_COUNT, INFINITE_TIMEOUT);

    if (file_descriptor_pool[STDIN_FD_POOL_ID].revents & POLLIN) {
      char *message = input(TCP);
      if (message == NULL) {
        message = input(UDP);
        send_udp_message(message);
        continue;
      }
      send_tcp_message(message);
    }
    if (file_descriptor_pool[TCP_SOCKET_FD_POOL_ID].revents & POLLIN) {
      js_socket_read(tcp_socket_fd, buffer, BUFSIZ);
      if (strcmp(buffer, EXIT_MESSAGE) == EQUAL_STRINGS) {
        printf(SIGINT_SYMBOL);
        raise(SIGINT);
      }
      printf("%s\n", buffer);
    }
    if (file_descriptor_pool[UDP_SOCKET_FD_POOL_ID].revents & POLLIN) {
      js_socket_receive_from(
        udp_socket_fd,
        buffer,
        MAX_MESSAGE_SIZE_BYTES,
        (sockaddr *) &server_address,
        &server_address_length,
        MSG_DONTWAIT);
      printf("%s\n", buffer);
    }
  }

  return EXIT_SUCCESS;
}

void init() {
  exited = false;

  atexit(handle_exit);
  js_init_sigint(handle_sigint);

  js_fd_nonblock(STDIN_FILENO);
  file_descriptor_pool[STDIN_FD_POOL_ID].fd     = STDIN_FILENO;
  file_descriptor_pool[STDIN_FD_POOL_ID].events = POLLIN;

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family      = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_address.sin_port        = htons(PORT);

  tcp_socket_fd = js_socket(AF_INET, SOCK_STREAM, 0);
  js_socket_connect(
    tcp_socket_fd, (sockaddr *) &server_address, sizeof(server_address));
  js_socket_read(tcp_socket_fd, message, MAX_MESSAGE_SIZE_BYTES);
  connection_id = strtol(message, NULL, 10);
  js_fd_nonblock(tcp_socket_fd);
  file_descriptor_pool[TCP_SOCKET_FD_POOL_ID].fd     = tcp_socket_fd;
  file_descriptor_pool[TCP_SOCKET_FD_POOL_ID].events = POLLIN;

  udp_socket_fd = js_socket(AF_INET, SOCK_DGRAM, 0);
  js_udp_hello(
    udp_socket_fd,
    connection_id,
    (const sockaddr *) &server_address,
    sizeof(server_address));
  js_fd_nonblock(udp_socket_fd);
  file_descriptor_pool[UDP_SOCKET_FD_POOL_ID].fd     = udp_socket_fd;
  file_descriptor_pool[UDP_SOCKET_FD_POOL_ID].events = POLLIN;
}

void handle_exit() {
  if (exited) {
    return;
  }

  if (tcp_socket_fd != SOCKET_UNDEFINED) {
    js_socket_write(tcp_socket_fd, EXIT_MESSAGE, EXIT_MESSAGE_LENGTH + 1);
    js_socket_close(&tcp_socket_fd);
  }
  exited = true;
}

void handle_sigint(int signal) {
  handle_exit();
}

char * input(int mode) {
  if (mode == TCP) {
    fgets(message, MAX_MESSAGE_SIZE_BYTES, stdin);
    message[strlen(message) - 1] = '\0';

    if (js_str_equal(message, UDP_MODE)) {
      info("Set UDP STDIN mode", INFO_TITLE);
      return NULL;
    }
  }
  else if (mode == UDP) {
    const int real_num_of_bytes = MAX_MESSAGE_SIZE_BYTES - UDP_HEADER_SIZE_BYTES;

    int i = 0;
    while (true) {
      js_poll(file_descriptor_pool, ONLY_STDIN_FD, INFINITE_TIMEOUT);

      char ch;
      while ((ch = fgetc(stdin)) != EOF && i < real_num_of_bytes) {
        message[i++] = ch;
      }

      if (feof(stdin)) {
        clearerr(stdin);
        break;
      }
    }
    message[i] = '\0';
  }

  return message;
}

void send_tcp_message(const char *message) {
  sprintf(buffer, "Sending message via TCP: \"%s\"", message);
  info(buffer, CLIENT_TITLE);
  js_socket_write(tcp_socket_fd, message, strlen(message) + 1);
}

void send_udp_message(const char *message) {
  info("Sending message via UDP", CLIENT_TITLE);
  sprintf(buffer, "%hhd%s", connection_id, message);
  js_socket_send_to(
    udp_socket_fd,
    buffer,
    strlen(buffer) + 1,
    (const sockaddr *) &server_address,
    sizeof(server_address),
    MSG_DONTWAIT);
}
