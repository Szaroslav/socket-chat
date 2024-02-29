#include "logger.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define REDBG "\e[0;41m"
#define WHTBG "\e[0;47m"
#define RESET "\e[0m"

void msg_internal(
    const char *message,
    const char *title,
    bool is_error,
    bool use_errno
) {
  char *title_color = WHTBG;
  if (is_error) {
    title_color = REDBG;
  }

  if (title != NULL) {
    fprintf(stdout, "%s %s %s %s\n", title_color, title, RESET, message);
  }
  else {
    fprintf(stdout, "%s\n", message);
  }

  if (is_error && use_errno) {
    const char *error_message = strerror(errno);
    fprintf(stderr, "%s Error %s %s\n", REDBG, RESET, error_message);
  }
}

void info(const char *message, const char *title) {
  msg_internal(message, title, false, false);
}

void error(const char *message, const char *title, bool use_errno) {
  msg_internal(message, title, true, use_errno);
}
