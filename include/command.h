#ifndef _COMMAND_H
#define _COMMAND_H

#include "hashtable.h"
#include "resp.h"
#include "sock.h"
#include <stdint.h>

typedef struct {
    HashTable* ht;
} Commander;

typedef void (*CommandHandler)(Commander *cmdr, SocketConnection *conn, RespObj *data);

typedef struct {
    char* name;
    CommandHandler proc;
} Command;

Commander *cmdr_create(HashTable *ht);
void cmdr_advance(Commander* cmdr, SocketConnection *conn, RespObj* o);

extern const Command cmd_registry[];
extern const uint64_t cmd_registry_len;

#endif