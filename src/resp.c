#include "resp.h"
#include "stringbuilder.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// todo: keep using arena for parsing-only data, use jemalloc/mimalloc for
// potential data (bulk strings, arrays)

// ======== parsing ===================

#define resp_check_overflow(src, end, status) \
    if (*src > end) { \
        *status = RESPERR_INCOMPLETE; \
        return NULL; \
    } \

static RespObj *resp_parse_str(char **src, char* end, int8_t *status, Arena *a);
static RespObj *resp_parse_err(char **src, char* end, int8_t *status, Arena *a);
static RespObj *resp_parse_int(char **src, char* end, int8_t *status, Arena *a);
static RespObj *resp_parse_bulk(char **src, char* end, int8_t *status, Arena *a);
static RespObj *resp_parse_arr(char **src, char* end, int8_t *status, Arena *a);

RespObj *resp_parse(char **src, char* end, int8_t *status, Arena *a) {
    RespType type = **src;
    switch (type) {
    case RESP_STR:
        return resp_parse_str(src, end, status, a);
    case RESP_ERR:
        return resp_parse_err(src, end, status, a);
    case RESP_INT:
        return resp_parse_int(src, end, status, a);
    case RESP_BULK:
        return resp_parse_bulk(src, end, status, a);
    case RESP_ARR:
        return resp_parse_arr(src, end, status, a);

    default: {
        *status = RESPERR_BAD_BODY;
        return NULL;
    }
    }
}

// simple strings: read until crlf
// +<message>\r\n
RespObj *resp_parse_str(char **src, char* end, int8_t *status, Arena *a) {
    (*src)++;       // skip type byte
    char *s = *src; // copy the pointer, to prevent moving the cursor

    uint64_t size = 0;
    while (*src < end && **src != '\r') {
        resp_check_overflow(src, end, status)
        size++;
        (*src)++;
    }
    *src += 2;
    resp_check_overflow(src, end, status)

    char *str = malloc(size);
    strncpy(str, s, size);

    RespObj *o = arena_alloc(a, sizeof(RespObj));
    o->value.string.buf = str;
    o->value.string.len = size;
    o->type = RESP_STR;

    return o;
}

// error strings: read until crlf
// -<message>\r\n
RespObj *resp_parse_err(char **src, char* end, int8_t *status, Arena *a) {
    (*src)++;       // skip type byte
    char *s = *src; // copy the pointer, to prevent moving the cursor

    uint64_t size = 0;
    while (**src != '\r' && **src + 1 != '\n') {
        resp_check_overflow(src, end, status)
        size++;
        (*src)++;
    }
    *src += 2;
    resp_check_overflow(src, end, status)

    char *str = arena_alloc(a, size+1);
    strncpy(str, s, size);
    str[size] = '\0';

    RespObj *o = arena_alloc(a, sizeof(RespObj));
    o->value.string.buf = str;
    o->value.string.len = size;
    o->type = RESP_ERR;

    return o;
}

// int: parse int
// :<number>\r\n
RespObj *resp_parse_int(char **src, char* end, int8_t *status, Arena *a) {
    (*src)++; // skip type byte
    int64_t val = 0;
    while (*src < end && **src != '\r') {
        resp_check_overflow(src, end, status)
        val = val * 10 + (**src - '0');
        (*src)++;
    }
    resp_check_overflow(src, end, status)
    *src += 2; // skip crln

    RespObj *o = arena_alloc(a, sizeof(RespObj));
    o->value.integer = val;
    o->type = RESP_INT;

    return o;
}

// bulk: parse by length
// $<length>\r\n<data>\r\n
RespObj *resp_parse_bulk(char **src, char* end, int8_t *status, Arena *a) {
    (*src)++; // skip type byte

    // get length
    int64_t len = 0;
    while (**src != '\r' && **src + 1 != '\n') {
        resp_check_overflow(src, end, status)
        len = len * 10 + (**src - '0');
        (*src)++;
    }
    *src += 2; // skip first crln
    resp_check_overflow(src, end, status)

    // get actual data
    // expect crln right after `length` amount of bytes
    // otherwise, throw an error
    if (*(*src + len) != '\r' || *(*src + len + 1) != '\n') {
        *status = RESPERR_INCOMPLETE;
        return NULL;
    }

    char *buf = arena_alloc(a, len+1);
    strncpy(buf, *src, len);
    buf[len] = '\0';

    (*src) += len + 2;

    RespObj *o = arena_alloc(a, sizeof(RespObj));
    o->value.string.len = len;
    o->value.string.buf = buf;
    o->type = RESP_BULK;

    return o;
}

// arr: parse by length + parse elements
// *<length>\r\n
RespObj *resp_parse_arr(char **src, char* end, int8_t *status, Arena *a) {
    (*src)++; // skip type byte

    // get length
    uint64_t len = 0;
    while (**src != '\r' && **src + 1 != '\n') {
        resp_check_overflow(src, end, status)
        len = len * 10 + (**src - '0');
        (*src)++;
    }
    *src += 2; // skip first crln
    resp_check_overflow(src, end, status)

    // fixme: region cant be freed incase of a parse error for elements
    RespObj **elements = arena_alloc(a, sizeof(RespObj *) * len);

    for (uint64_t i = 0; i < len; i++) {
        RespObj *e = resp_parse(src, end, status, a);
        if (*status != 0 || e == NULL) {
            return NULL;
        }

        elements[i] = e;
    }

    RespObj *o = arena_alloc(a, sizeof(RespObj));
    o->value.array.len = len;
    o->value.array.items = elements;
    o->type = RESP_ARR;

    return o;
}

// ======== emitting ===================

static void resp_marshal_obj(RespObj *o, StringBuilder *sb);
static void resp_marshal_str(RespObj *o, StringBuilder *sb);
static void resp_marshal_err(RespObj *o, StringBuilder *sb);
static void resp_marshal_int(RespObj *o, StringBuilder *sb);
static void resp_marshal_bulk(RespObj *o, StringBuilder *sb);
static void resp_marshal_arr(RespObj *o, StringBuilder *sb);

char *resp_marshal(RespObj *o, uint64_t *sizeptr) {
    StringBuilder *sb = sb_create();

    resp_marshal_obj(o, sb);

    *sizeptr = sb->length;
    char *res = sb_concat(sb);
    sb_free(sb);

    return res;
}

void resp_marshal_obj(RespObj *o, StringBuilder *sb) {
    switch (o->type) {
    case RESP_STR:
        resp_marshal_str(o, sb);
        break;
    case RESP_ERR:
        resp_marshal_err(o, sb);
        break;
    case RESP_INT:
        resp_marshal_int(o, sb);
        break;
    case RESP_BULK:
        resp_marshal_bulk(o, sb);
        break;
    case RESP_ARR:
        resp_marshal_arr(o, sb);
        break;
    }
}

void resp_marshal_str(RespObj *o, StringBuilder *sb) {
    sb_appendf(sb, "+%s\r\n", o->value.string.buf);
}

void resp_marshal_err(RespObj *o, StringBuilder *sb) {
    sb_appendf(sb, "-%s\r\n", o->value.string.buf);
}

void resp_marshal_int(RespObj *o, StringBuilder *sb) {
    sb_appendf(sb, ":%d\r\n", o->value.integer);
}

void resp_marshal_bulk(RespObj *o, StringBuilder *sb) {
    sb_appendf(sb, "$%ld\r\n%s\r\n", o->value.string.len, o->value.string.buf);
}

void resp_marshal_arr(RespObj *o, StringBuilder *sb) {
    sb_appendf(sb, "*%ld\r\n", o->value.array.len);
    for (uint64_t i = 0; i < o->value.array.len; i++) {
        resp_marshal_obj(o->value.array.items[i], sb);
    }
}

// ======== pretty printing ===================

static void resp_prettyprint_indent(RespObj *obj, FILE *f, uint8_t i);
static void resp_prettyprint_type(RespType t, FILE *f);

#define resp_prettyprint_spaces(l, f)                                          \
    for (int i = 0; i < l; i++)                                                \
    fputc(' ', f)

void resp_prettyprint(RespObj *obj, FILE *f) {
    resp_prettyprint_indent(obj, f, 0);
}

void resp_prettyprint_indent(RespObj *obj, FILE *f, uint8_t i) {
    switch (obj->type) {
    case RESP_ERR:
    case RESP_BULK:
    case RESP_STR: {
        resp_prettyprint_spaces(i * 4, f);
        fputs("type: ", f);
        resp_prettyprint_type(obj->type, f);
        fputc('\n', f);

        resp_prettyprint_spaces(i * 4, f);
        fprintf(f, "length: %lu\n", obj->value.string.len);

        resp_prettyprint_spaces(i * 4, f);
        fputs("data: ", f);
        fputs(obj->value.string.buf, f);
        fputc('\n', f);
        break;
    }

    case RESP_INT: {
        resp_prettyprint_spaces(i * 4, f);
        fputs("type: ", f);
        resp_prettyprint_type(obj->type, f);
        fputc('\n', f);

        resp_prettyprint_spaces(i * 4, f);
        fprintf(f, "value: %ld\n", obj->value.integer);
        break;
    }

    case RESP_ARR: {
        resp_prettyprint_spaces(i * 4, f);
        fputs("type: ", f);
        resp_prettyprint_type(obj->type, f);
        fputc('\n', f);

        resp_prettyprint_spaces(i * 4, f);
        fprintf(f, "length: %lu\n", obj->value.array.len);

        resp_prettyprint_spaces(i * 4, f);
        fputs("data: ", f);
        for (int i = 0; i < obj->value.array.len; i++) {
            resp_prettyprint_indent(obj->value.array.items[i], f, i + 1);
        }
        break;
    }
    }
}

void resp_prettyprint_type(RespType t, FILE *f) {
    switch (t) {
    case RESP_STR:
        fprintf(f, "string");
        break;
    case RESP_ERR:
        fprintf(f, "error");
        break;
    case RESP_INT:
        fprintf(f, "integer");
        break;
    case RESP_BULK:
        fprintf(f, "bulk string");
        break;
    case RESP_ARR:
        fprintf(f, "array");
        break;
    }
}

// ========= cloning =============

RespObj *resp_obj_clone_malloc(RespObj* obj) {
    RespObj* o = malloc(sizeof(RespObj));

    switch (obj->type) {
        case RESP_INT: {
            o->type = RESP_INT;
            o->value.integer = obj->value.integer;
        }
        case RESP_ERR: {
            o->type = RESP_ERR;
            o->value.string.len = obj->value.string.len;
            o->value.string.buf = malloc(o->value.string.len);
            strncpy(o->value.string.buf, obj->value.string.buf, o->value.string.len);
        }
        case RESP_STR: {
            o->type = RESP_STR;
            o->value.string.len = obj->value.string.len;
            o->value.string.buf = malloc(o->value.string.len);
            strncpy(o->value.string.buf, obj->value.string.buf, o->value.string.len);
        }
        case RESP_ARR: {
            o->type = RESP_ARR;
            o->value.array.len = obj->value.array.len;
            o->value.array.items = malloc(o->value.array.len * sizeof(RespObj));
            memcpy(o->value.array.items, obj->value.array.items, o->value.array.len * sizeof(RespObj));
        }
        case RESP_BULK: {
            o->type = RESP_BULK;
            o->value.string.len = obj->value.string.len;
            o->value.string.buf = malloc(o->value.string.len);
            strncpy(o->value.string.buf, obj->value.string.buf, o->value.string.len);
        }
    }
    return o;
}

void resp_obj_free(RespObj *obj) {
    switch (obj->type) {
        case RESP_ERR:
        case RESP_BULK:
        case RESP_STR: {
            free(obj->value.string.buf);
            break;
        }
        case RESP_ARR: {
            // fixme: wait.. what if the items are all allocated separately?
            free(obj->value.array.items);
            break;
        }

        default: break;
    }

    free(obj);
}