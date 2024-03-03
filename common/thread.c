#include "thread.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>

#define THREAD_SUCCESS 0

void js_thread(
    pthread_t *thread,
    const pthread_attr_t *attributes,
    void *(*routine)(void *),
    void *args
) {
  if (pthread_create(thread, attributes, routine, args) != THREAD_SUCCESS) {
    error("Thread creation failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Thread creation succeeded", SUCCESS_TITLE);
}

void js_thread_join(pthread_t thread, void **exit_status) {
  if (pthread_join(thread, exit_status) != THREAD_SUCCESS) {
    error("Thread joining failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("Thread joining succeeded", SUCCESS_TITLE);
}
