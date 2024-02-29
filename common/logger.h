#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdbool.h>

void info(const char *message, const char *title);
void error(const char *message, const char *title, bool use_errno);

#endif // __LOGGER_H__
