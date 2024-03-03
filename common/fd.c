#include "fd.h"
#include "logger.h"

#include <fcntl.h>
#include <stdlib.h>

#define FILE_CONTROL_FAILURE -1

void js_fd_nonblock(int file_descriptor) {
  if (fcntl(file_descriptor, F_SETFL, O_NONBLOCK) == FILE_CONTROL_FAILURE) {
    error("Non-blocking file descriptor set-up failed", ERROR_TITLE, true);
    exit(FILE_CONTROL_FAILURE);
  }
  success("Non-blocking file descriptor set-up succeeded", SUCCESS_TITLE);
}
