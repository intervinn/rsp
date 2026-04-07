#ifndef _SOCK_H
#define _SOCK_H

#include "resp.h"
#include <netinet/in.h>
#include <stdint.h>
#include <sys/epoll.h>

typedef struct {
    char* rdbuf;
    uint64_t size;
    uint64_t read;
    int fd;
} SocketConnection;

typedef void (*OnMessage)(SocketConnection* conn, RespObj* obj, Arena* a);

typedef struct {
    struct epoll_event* events;
    struct epoll_event event;
    struct sockaddr_in addr;
    uint64_t bufdefault;

    OnMessage on_message;

    uint32_t max_events;
    int32_t serve_fd;
    int32_t epoll_fd;

    uint16_t port;
} SocketListener;

SocketListener* sock_create(uint16_t port, uint32_t max_events, uint64_t bufsize, OnMessage on_message);
void sock_listen(SocketListener* s);
void sock_free(SocketListener* s);

#endif