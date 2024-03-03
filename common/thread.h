#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

void js_thread(
  pthread_t *thread,
  const pthread_attr_t *attributes,
  void *(*routine)(void *),
  void *args);
void js_thread_join(pthread_t thread, void **exit_status);

#endif // __THREAD_H__
