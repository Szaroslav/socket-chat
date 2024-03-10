#include "client.h"

#include "../common/logger.h"
#include "../common/byte.h"
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
#define FILE_DESCRIPTORS_COUNT 4
#define STDIN_FD_POOL_ID       0
#define TCP_SOCKET_FD_POOL_ID  1
#define UDP_SOCKET_FD_POOL_ID  2
#define MTC_SOCKET_FD_POOL_ID  3
#define ONLY_STDIN_FD          1
#define UDP_MODE               "U"
#define MTC_MODE               "M"

static bool               exited;
static char               buffer[BUFSIZ];
static char               message[MAX_MESSAGE_SIZE_BYTES];
static int                tcp_socket_fd;
static int                udp_socket_fd;
static int                mtc_socket_fd;
static int                active_udp_socket_fd;
static int                connection_id;
static struct sockaddr_in server_address;
static struct sockaddr_in multicast_address;
static struct pollfd      file_descriptor_pool[FILE_DESCRIPTORS_COUNT];

void   init                  ();
void   handle_exit           ();
void   handle_sigint         (int signal);
char * input                 (int mode);
void   send_tcp_message      (const char *message);
void   send_udp_message      (int socket_fd, struct sockaddr_in *destination, const char *message);
char * receive_tcp_message   (int socket_fd);
char * receive_udp_message   (int socket_fd);

int main() {
  init();

  while (true) {
    js_poll(file_descriptor_pool, FILE_DESCRIPTORS_COUNT, INFINITE_TIMEOUT);

    if (file_descriptor_pool[STDIN_FD_POOL_ID].revents & POLLIN) {
      char *message = input(TCP);
      if (message == NULL) {
        message = input(UDP);

        if (active_udp_socket_fd == SOCKET_UNDEFINED) {
          warn("UDP socket is undefined", WARNING_TITLE);
          continue;
        }
        send_udp_message(active_udp_socket_fd, active_udp_socket_fd == udp_socket_fd ? &server_address : &multicast_address, message);

        continue;
      }
      if (js_str_equal(message, "")) {
        continue;
      }
      send_tcp_message(message);
    }
    if (file_descriptor_pool[TCP_SOCKET_FD_POOL_ID].revents & POLLIN) {
      const char *buffer = receive_tcp_message(tcp_socket_fd);
      printf("%s\n", buffer);
    }
    if (file_descriptor_pool[UDP_SOCKET_FD_POOL_ID].revents & POLLIN) {
      const char *buffer = receive_udp_message(udp_socket_fd);
      printf("%s\n", buffer);
    }
    if (file_descriptor_pool[MTC_SOCKET_FD_POOL_ID].revents & POLLIN) {
      const char *buffer = receive_udp_message(mtc_socket_fd);
      printf("%s\n", buffer);
    }
  }

  return EXIT_SUCCESS;
}

void init() {
  active_udp_socket_fd = SOCKET_UNDEFINED;
  exited               = false;

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
  connection_id = bytes_to_int((byte *) message);
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

  memset(&multicast_address, 0, sizeof(multicast_address));
  multicast_address.sin_family      = AF_INET;
  multicast_address.sin_addr.s_addr = INADDR_ANY;
  multicast_address.sin_port        = htons(PORT + 1);

  mtc_socket_fd = js_socket_multicast(
    AF_INET, SOCK_DGRAM, 0, MULTICAST_ADDRESS_IPV4, &multicast_address.sin_addr);
  js_socket_bind(
    mtc_socket_fd, (const struct sockaddr *) &multicast_address, sizeof(multicast_address));
  js_fd_nonblock(mtc_socket_fd);
  file_descriptor_pool[MTC_SOCKET_FD_POOL_ID].fd     = mtc_socket_fd;
  file_descriptor_pool[MTC_SOCKET_FD_POOL_ID].events = POLLIN;
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
    if (fgets(message, MAX_MESSAGE_SIZE_BYTES, stdin) == NULL) {
      clearerr(stdin);
      return "";
    }
    message[strlen(message) - 1] = '\0';

    if (js_str_equal(message, UDP_MODE)) {
      info("Set UDP STDIN mode", INFO_TITLE);
      active_udp_socket_fd = udp_socket_fd;
      return NULL;
    }
    else if (js_str_equal(message, MTC_MODE)) {
      info("Set MULTICAST STDIN mode", INFO_TITLE);
      active_udp_socket_fd = mtc_socket_fd;
      return NULL;
    }

    active_udp_socket_fd = SOCKET_UNDEFINED;
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
        break;
      }
    }
    message[i] = '\0';
  }

  clearerr(stdin);

  return message;
}

void send_tcp_message(const char *message) {
  sprintf(buffer, "Sending message via TCP: \"%s\"", message);
  info(buffer, CLIENT_TITLE);
  js_socket_write(tcp_socket_fd, message, strlen(message) + 1);
}

void send_udp_message(int socket_fd, struct sockaddr_in *destination, const char *message) {
  info("Sending message via UDP", CLIENT_TITLE);
  int_to_bytes((byte *) buffer, connection_id);
  sprintf(buffer + UDP_HEADER_SIZE_BYTES, "%s", message);
  js_socket_send_to(
    socket_fd,
    buffer,
    UDP_HEADER_SIZE_BYTES + strlen(buffer + UDP_HEADER_SIZE_BYTES) + 1,
    (const sockaddr *) destination,
    sizeof(*destination),
    MSG_DONTWAIT);
}

char * receive_tcp_message(int socket_fd) {
  js_socket_read(socket_fd, message, MAX_MESSAGE_SIZE_BYTES);
  if (strcmp(message, EXIT_MESSAGE) == EQUAL_STRINGS) {
    printf(SIGINT_SYMBOL);
    raise(SIGINT);
  }
  return message;
}

char * receive_udp_message(int socket_fd) {
  int received_bytes = js_socket_receive_from(
    socket_fd,
    message,
    MAX_MESSAGE_SIZE_BYTES,
    NULL,
    NULL,
    MSG_DONTWAIT);
  if (message[received_bytes - 1] == '\n') {
    message[received_bytes - 1] = '\0';
  }

  const int client_id = bytes_to_int((byte *) message);
  snprintf(
    buffer,
    BUFSIZ,
    "%s %s%2d %s\n%s",
    WHTBG,
    CLIENT_TITLE_WITH_ID,
    client_id,
    RESET,
    message + UDP_HEADER_SIZE_BYTES);

  return buffer;
}
