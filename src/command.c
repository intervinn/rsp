#include "command.h"
#include "hashtable.h"
#include "resp.h"
#include <stdlib.h>
#include <string.h>

const Command cmd_registry[] = {
    {"GET", cmd_get},
    {"SET", cmd_set},
    {"PING", cmd_ping}
};
const uint64_t cmd_registry_len = sizeof(cmd_registry) / sizeof(Command);


static const char* ERR_BADBODY = "-ERR bad body\r\n";
static const char* ERR_UNKNOWNCMD = "-ERR unknown command\r\n";

Commander *cmdr_create(HashTable *ht) {
    Commander *c = malloc(sizeof(Commander));
    c->ht = ht;
    return c;
}

void cmdr_advance(Commander* cmdr, SocketConnection *conn, RespObj* o) {
    if (o->type != RESP_ARR || o->value.array.items[0]->type != RESP_BULK) {
        send(conn->fd, ERR_BADBODY, 16, 0);
        return;
    }

    char* cmdname = o->value.array.items[0]->value.string.buf;

    for (uint64_t i = 0; i < cmd_registry_len; i++) {
        Command cmd = cmd_registry[i];
        if (strcmp(cmdname, cmd.name) == 0) {
            cmd.proc(cmdr, conn, o);
            return;
        }
    }

    send(conn->fd, ERR_UNKNOWNCMD, 23, 0);
}

void cmd_get(Commander *cmdr, SocketConnection *conn, RespObj *data) {

}

void cmd_set(Commander *cmdr, SocketConnection *conn, RespObj *data) {

}

void cmd_del(Commander *cmdr, SocketConnection *conn, RespObj *data) {

}

void cmd_exists(Commander *cmdr, SocketConnection *conn, RespObj *data) {

}

void cmd_ping(Commander *cmdr, SocketConnection *conn, RespObj *data) {
    send(conn->fd, "+PONG\r\n", 8, 0);
}