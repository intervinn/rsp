#include "sock.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>

struct _SocketListener {
    struct epoll_event* events;
    char* buf;

    struct epoll_event event;
    struct sockaddr_in addr;

    uint64_t bufsize;

    uint32_t max_events;
    int32_t serve_fd;
    int32_t epoll_fd;

    uint16_t port;
};

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

SocketListener* sock_create(uint16_t port, uint32_t max_events, uint64_t bufsize) {
    SocketListener* s = malloc(sizeof(SocketListener));
    if (s == NULL) 
        return NULL;

    s->port = port;
    s->max_events = max_events;
    s->bufsize = bufsize;

    s->events = malloc(max_events * sizeof(struct epoll_event));
    if (s->events == NULL) {
        free(s);
        return NULL;
    }

    s->buf = malloc(bufsize);
    if (s->buf == NULL) {
        free(s->events);
        free(s);
        return NULL;
    }

    return s;
}

static int sock_bind(SocketListener* s) {
    int opt = 1;

    // create socket
    s->serve_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (s->serve_fd == -1) {
        return -1;
    }
    setsockopt(s->serve_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&s->addr, 0, sizeof(s->addr));
    s->addr.sin_family = AF_INET;
    s->addr.sin_addr.s_addr = INADDR_ANY;
    s->addr.sin_port = htons(s->port);

    int status = bind(s->serve_fd, (struct sockaddr*)&s->addr, sizeof(s->addr));
    if (status == -1) {
        return -1; // also see errno, if this fails
    }

    listen(s->serve_fd, SOMAXCONN);

    // create poll instance
    s->epoll_fd = epoll_create1(0);

    // register server socket to read events
    s->event.events = EPOLLIN;
    s->event.data.fd = s->serve_fd;
    epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, s->serve_fd, &s->event);

    return 0;
}

static int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    int fd = accept(sockfd, addr, addrlen);
    if (fd == -1) {
        return -1;
    }

    if (flags & SOCK_CLOEXEC) {
        int f = fcntl(fd, F_GETFD);
        if (f == -1 || fcntl(fd, F_SETFD, f | FD_CLOEXEC) == -1) {
            return -1;
        }
    }

    if (flags & SOCK_NONBLOCK) {
        int f = fcntl(fd, F_GETFL);
        if (f == -1 || fcntl(fd, F_SETFL, f | O_NONBLOCK) == -1) {
            return -1;
        }
    }

    return fd;
}

static void sock_handle(SocketListener* s) {

}

void sock_listen(SocketListener* s) {
    while (true) {
        int n = epoll_wait(s->epoll_fd, s->events, s->max_events, -1);
        for (int i = 0; i < n; i++) {
            int fd = s->events[i].data.fd;

            if (fd == s->serve_fd) {
                // accept all pending connections
                while (true) {
                    struct sockaddr_in caddr;
                    socklen_t clen = sizeof(caddr);
                    int cfd = accept4(s->serve_fd, (struct sockaddr*)&caddr, &clen, SOCK_NONBLOCK);

                    if (cfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("accept4");
                        break;
                    }

                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
                    printf("[+] %s:%d (fd=%d)\n", ip, ntohs(caddr.sin_port), cfd);

                    s->event.events = EPOLLIN | EPOLLET; // edge-triggered
                    s->event.data.fd = cfd;
                    epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, cfd, &s->event);
                }
            } else if (s->events[i].events & (EPOLLERR | EPOLLHUP)) {
                printf("[-] fd=%d hung up\n", fd);
                epoll_ctl(s->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            } else if (s->events[i].events & EPOLLIN) {
                // edge-triggered: must drain all available data
                while (true) {
                    int64_t len = recv(fd, s->buf, s->bufsize, 0);
                    if (len > 0) {
                        send(fd, s->buf, len, 0); // echo
                    } else if (len == 0) {
                        // client closed connection
                        epoll_ctl(s->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        epoll_ctl(s->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        break;
                    }
                }
            }
        }
    }

    close(s->epoll_fd);
    close(s->serve_fd);
}

void sock_free(SocketListener* s) {
    free(s->events);
    free(s->buf);
    free(s);
}

