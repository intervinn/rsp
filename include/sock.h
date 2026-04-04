#ifndef _SOCK_H
#define _SOCK_H

#include <netinet/in.h>

#include <stdint.h>

typedef struct _SocketListener SocketListener;
typedef struct _SocketConnection SocketConnection;

SocketListener* sock_create(uint16_t port, uint32_t max_events, uint64_t bufsize);
void sock_listen(SocketListener* s);
void sock_free(SocketListener* s);

#endif