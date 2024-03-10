#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdbool.h>

#define INFO_TITLE    "Info     "
#define SUCCESS_TITLE "Success  "
#define WARNING_TITLE "Warning  "
#define ERROR_TITLE   "Error    "

#define WHTBG         "\e[0;47m"
#define BGRBG         "\e[0;102m"
#define YELBG         "\e[0;43m"
#define REDBG         "\e[0;41m"
#define RESET         "\e[0m"

void info    (const char *message, const char *title                );
void info_wnl(const char *message, const char *title                );
void success (const char *message, const char *title                );
void warn    (const char *message, const char *title                );
void error   (const char *message, const char *title, bool use_errno);

#endif // __LOGGER_H__
