this is a simple implementation of a redis server

## potential improvements
* epoll -> libuv (crossplatform)
* arena,malloc -> jemalloc/mimalloc (avoid copying for useful strings)
* rehash without freezing