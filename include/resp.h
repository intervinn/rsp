#ifndef _RESP_H
#define _RESP_H

#include "arena.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum { RESPERR_BAD_BODY, RESPERR_OUTRANGE } RespStatus;

typedef enum __attribute__((packed)) {
    RESP_STR = '+',
    RESP_ERR = '-',
    RESP_INT = ':',
    RESP_BULK = '$',
    RESP_ARR = '*',
} RespType;

typedef struct RespObj {
    RespType type;
    union {
        int64_t integer;

        struct {
            uint64_t len;
            char *buf;
        } string;

        struct {
            uint64_t len;
            struct RespObj **items;
        } array;
    } value;
} RespObj;

RespObj *resp_parse(char **src, int8_t *status, Arena *a);
char *resp_marshal(RespObj *o, uint64_t *sizeptr);
void resp_prettyprint(RespObj *obj, FILE *f);

#endif // _RESP_H