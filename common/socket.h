#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/socket.h>

#define SOCKET_SUCCESS         0
#define SOCKET_FAILURE         -1
#define SOCKET_UNDEFINED       -1
#define SOCKET_READ_FAILURE    -1
#define SOCKET_RECEIVE_FAILURE -1
#define SOCKET_WRITE_FAILURE   -1
#define SOCKET_SEND_FAILURE    -1
#define MAX_MESSAGE_SIZE_BYTES 4096
#define UDP_HEADER_SIZE_BYTES  1

typedef enum socket_type {
  TCP = SOCK_STREAM,
  UDP = SOCK_DGRAM,
} socket_type;

typedef struct sockaddr sockaddr;

int  js_socket(int domain, int type, int protocol);
void js_socket_option(
  int socket_fd,
  int level,
  int name,
  const void *value,
  socklen_t length);
int  js_socket_read(int socket_fd, char *buffer, int max_bytes_to_read);
int  js_socket_receive_from(
  int socket_fd,
  char *buffer,
  int max_bytes_to_receive,
  sockaddr *source_address,
  socklen_t *address_length,
  int flags);
int  js_socket_write(int socket_fd, const char *buffer, int max_bytes_to_write);
int  js_socket_send_to(
  int socket_fd,
  const char *buffer,
  int max_bytes_to_send,
  const sockaddr *destination_address,
  socklen_t address_length,
  int flags);
void js_socket_bind(int socket_fd, const sockaddr *address, socklen_t address_length);
void js_socket_listen(int socket_fd, int maximum_pending_connections_count);
int  js_socket_accept(int socket_fd, sockaddr *address, socklen_t *address_length);
void js_socket_connect(int socket_fd, const sockaddr *address, socklen_t address_length);
void js_socket_close(int *socket_fd);

#endif // __SOCKET_H__
