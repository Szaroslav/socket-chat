#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#define          CONNECTION_SUCCESS    0
#define          CONNECTION_FAILURE    -1
#define          HELLO_MESSAGE         "HELLO"
extern const int HELLO_MESSAGE_LENGTH;
#define          EXIT_MESSAGE          "EXIT"
extern const int EXIT_MESSAGE_LENGTH;

typedef struct connection {
  bool      active;
  int       socket_fd;
  pthread_t thread;
} connection;

typedef struct connection_params {
  int id;
  int socket_fd;
} connection_params;

bool js_is_hello_message(const char *message);
void js_init_connection_array(connection *connections, int size);
int  js_connection(connection *connections, int size, int socket_fd);
int  js_connection_id(connection *connections, int size);
void js_prepare_connection_termination(connection *connections, int id);
void js_terminate_connection(connection *connections, int id);

#endif // __CONNECTION_H__
