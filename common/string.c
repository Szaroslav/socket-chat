#include "string.h"
#include <string.h>

bool js_str_equal(const char *arg1, const char *arg2) {
  return strcmp(arg1, arg2) == STRING_EQUAL;
}
