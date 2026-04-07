#include "command.h"
#include "hashtable.h"
#include "resp.h"
#include "sock.h"
#include <stdlib.h>
#include <string.h>

static void cmd_get(Commander* cmdr, SocketConnection* conn, RespObj *o);
static void cmd_set(Commander* cmdr, SocketConnection* conn, RespObj *o);
static void cmd_ping(Commander* cmdr, SocketConnection* conn, RespObj *o);
static void cmd_del(Commander* cmdr, SocketConnection* conn, RespObj *o);
static void cmd_exists(Commander* cmdr, SocketConnection* conn, RespObj *o);

const Command cmd_registry[] = {
    {"GET", cmd_get},
    {"SET", cmd_set},
    {"PING", cmd_ping}
};
const uint64_t cmd_registry_len = sizeof(cmd_registry) / sizeof(Command);

static const char* ERR_BADBODY = "-ERR bad body\r\n";
static const char* ERR_UNKNOWNCMD = "-ERR unknown command\r\n";

static const char* RES_OK = "+OK\r\n";
static const char* RES_NULL = "$-1\r\n";

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
    if (data->value.array.len != 2) {
        send(conn->fd, ERR_BADBODY, 16, 0);
        return;
    }

    RespObj* key = data->value.array.items[1];
    RespObj* value = ht_get(cmdr->ht, key->value.string.buf);
    if (value == NULL) {
        send(conn->fd, RES_NULL, 6, 0);
    } else {
        uint64_t size;
        char* str = resp_marshal(value, &size);
        send(conn->fd, str, size, 0);
        free(str);
    }
}

void cmd_set(Commander *cmdr, SocketConnection *conn, RespObj *data) {
    if (data->value.array.len < 3) {
        send(conn->fd, ERR_BADBODY, 16, 0);
        return;
    }

    RespObj* key_parsed = data->value.array.items[1];
    RespObj* val_parsed = data->value.array.items[2];


    if (key_parsed->type != RESP_STR && key_parsed->type != RESP_BULK) {
        send(conn->fd, ERR_BADBODY, 16, 0);
        return;
    }

    char* key = strdup(key_parsed->value.string.buf);
    RespObj* val = resp_obj_clone_malloc(val_parsed);

    ht_set(cmdr->ht, key, val);
    send(conn->fd, RES_OK, 6, 0);
}

void cmd_del(Commander *cmdr, SocketConnection *conn, RespObj *data) {

}

void cmd_exists(Commander *cmdr, SocketConnection *conn, RespObj *data) {

}

void cmd_ping(Commander *cmdr, SocketConnection *conn, RespObj *data) {
    send(conn->fd, "+PONG\r\n", 8, 0);
}