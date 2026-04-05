#ifndef _SOCK_H
#define _SOCK_H

#include "hashtable.h"
#include <netinet/in.h>

#include <stdint.h>

typedef struct _SocketListener SocketListener;
typedef struct {
    char* rdbuf;
    uint64_t size;
    uint64_t read;
    int fd;
} SocketConnection;

SocketListener* sock_create(HashTable *ht, uint16_t port, uint32_t max_events, uint64_t bufsize);
void sock_listen(SocketListener* s);
void sock_free(SocketListener* s);

#endif