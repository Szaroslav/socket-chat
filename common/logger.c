#include "logger.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define WHTBG "\e[0;47m"
#define BGRBG "\e[0;102m"
#define YELBG "\e[0;43m"
#define REDBG "\e[0;41m"
#define RESET "\e[0m"

typedef enum message_type {
  INFO,
  SUCCESS,
  WARN,
  ERROR,
} message_type;

void msg_internal(
    const char *message,
    const char *title,
    message_type type,
    bool use_errno
) {
  char *title_color = WHTBG;
  switch (type) {
    case SUCCESS: {
      title_color = BGRBG;
      break;
    }
    case WARN: {
      title_color = YELBG;
      break;
    }
    case ERROR: {
      title_color = REDBG;
      break;
    }
  }

  if (title != NULL) {
    fprintf(stdout, "%s %s %s %s\n", title_color, title, RESET, message);
  }
  else {
    fprintf(stdout, "%s\n", message);
  }

  if (type == ERROR && use_errno) {
    const char *error_message = strerror(errno);
    fprintf(stderr, "%s %s %s %s\n", REDBG, ERROR_TITLE, RESET, error_message);
  }
}

void info(const char *message, const char *title) {
  msg_internal(message, title, INFO, false);
}

void success(const char *message, const char *title) {
  msg_internal(message, title, SUCCESS, false);
}

void warn(const char *message, const char *title) {
  msg_internal(message, title, WARN, false);
}

void error(const char *message, const char *title, bool use_errno) {
  msg_internal(message, title, ERROR, use_errno);
}
