#include "socket.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int js_socket(int domain, int type, int protocol) {
  const int socket_fd = socket(domain, type, protocol);
  if (socket_fd == SOCKET_FAILURE) {
      error("Socket creation failed", ERROR_TITLE, true);
      exit(EXIT_FAILURE);
  }
  success("Socket creation succeeded", SUCCESS_TITLE);
  return socket_fd;
}

int js_socket_read(int socket_fd, char *buffer, int max_bytes_to_read) {
  const int read_bytes = read(socket_fd, buffer, max_bytes_to_read);
  if (read_bytes == SOCKET_READ_FAILURE) {
    error("Socket reading failed", ERROR_TITLE, true);
    exit(SOCKET_READ_FAILURE);
  }
  success("Socket reading succeeded", SUCCESS_TITLE);
  return read_bytes;
}

int js_socket_write(int socket_fd, const char *buffer, int max_bytes_to_write) {
  const int write_bytes = write(socket_fd, buffer, max_bytes_to_write);
  if (write_bytes == SOCKET_WRITE_FAILURE) {
    error("Socket writing failed", ERROR_TITLE, true);
    exit(SOCKET_WRITE_FAILURE);
  }
  success("Socket writing succeeded", SUCCESS_TITLE);
  return write_bytes;
}

void js_socket_bind(int socket_fd, const struct sockaddr *address, socklen_t address_length) {
  if (bind(socket_fd, address, address_length) != SOCKET_SUCCESS) {
    error("Socket binding failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Socket binding succeeded", SUCCESS_TITLE);
}

void js_socket_listen(int socket_fd, int maximum_pending_connections_count) {
  if (listen(socket_fd, maximum_pending_connections_count) != SOCKET_SUCCESS) {
    error("Socket listening failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Socket listening succeeded", SUCCESS_TITLE);
}

int js_socket_accept(int socket_fd, struct sockaddr *address,socklen_t *address_length) {
  const int connection_fd = accept(socket_fd, address, address_length);
  if (connection_fd < SOCKET_SUCCESS) {
    error("Socket accepting failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Socket accepting succeeded", SUCCESS_TITLE);
  return connection_fd;
}

void js_socket_connect(
    int socket_fd,
    const struct sockaddr *address,
    socklen_t address_length
) {
  if (connect(socket_fd, address, address_length) != SOCKET_SUCCESS) {
    error("Socket connecting failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Socket connecting succeeded", SUCCESS_TITLE);
}

void js_socket_close(int *socket_fd) {
  if (*socket_fd == SOCKET_UNDEFINED) {
    warn("Socket is undefined", WARNING_TITLE);
    return;
  }

  if (close(*socket_fd) < 0) {
    error("Socket closing failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  *socket_fd = SOCKET_UNDEFINED;
  success("Socket closing succeeded", SUCCESS_TITLE);
}
