#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#define SIGINT_SYMBOL "^C"

void js_init_sigint(void (*callback)(int));

#endif // __SIGNAL_H__
