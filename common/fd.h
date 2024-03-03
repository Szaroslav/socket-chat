#ifndef __FILE_DESCRIPTOR_H__
#define __FILE_DESCRIPTOR_H__

#define _GNU_SOURCE
#include <poll.h>

#define INFINITE_TIMEOUT -1

void js_fd_nonblock(int file_descriptor);
void js_poll(struct pollfd *fd_pool, nfds_t fd_pool_size, int timeout);

#endif // __FILE_DESCRIPTOR_H__
