#include "command.h"
#include "hashtable.h"
#include "resp.h"
#include "sock.h"

#include <stdint.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

static HashTable *ht;
static Commander *cmdr;

void on_message(SocketConnection *conn, RespObj *obj, Arena* a) {
    cmdr_advance(cmdr, conn, obj);
}

int main() {
    printf("now we're starting the server...\n");
    
    ht = ht_create();
    cmdr = cmdr_create(ht);
    SocketListener* s = sock_create(6379, 100, 1028, on_message);
    sock_listen(s);

    sock_free(s);
    free(cmdr);
    free(ht);
    
    printf("app has shut down\n");
    return 0;
}