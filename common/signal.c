#include "signal.h"
#include "logger.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#define SIGNAL_SUCCESS 0
static void (*callback)(int);

void handle_signint(int signal);

void js_init_sigint(void (*c)(int)) {
  struct sigaction action = { 0 };
  action.sa_handler = handle_signint;
  callback = c;

  if (sigaction(SIGINT, &action, NULL) != SIGNAL_SUCCESS) {
    error("SIGINT handler creation failed", ERROR_TITLE, true);
    exit(EXIT_FAILURE);
  }
  success("SIGINT handler creation succeeded", SUCCESS_TITLE);
}

void handle_signint(int signal) {
  printf("\n");
  callback(signal);
  exit(EXIT_SUCCESS);
}
