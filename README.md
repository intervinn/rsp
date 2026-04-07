this is a simple implementation of a redis server

## potential improvements
* epoll -> libuv (crossplatform)
* arena,malloc -> jemalloc/mimalloc (avoid copying for useful strings)
* rehash without freezing
* the application entirely is pretty leaky

## resources
* [redis protocol spec](https://redis.io/docs/latest/develop/reference/protocol-spec/)
* [concurrent epoll server](https://kaleid-liner.github.io/blog/2019/06/02/epoll-web-server.html/)
* [how to implement a hash table](https://benhoyt.com/writings/hash-table-in-c/)
* [build redis from scratch (go)](https://www.build-redis-from-scratch.dev/en/introduction)
* [beej's guide to network programming](https://www.beej.us/guide/bgnet/html/)

