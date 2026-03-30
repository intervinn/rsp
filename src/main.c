#include "resp.h"

#include <stdint.h>
#include <string.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

int main() {
    char *strings[] = {"+OK\r\n", ":1000\r\n", "$5\r\nhello\r\n",
                       "*2\r\n$3\r\nGET\r\n$5\r\nmykey\r\n"};
    uint64_t lenstrings = 4;
    uint64_t size = 0;
    int8_t status = 0;
    Arena ar = {0};

    printf("trying out strings...\n");
    for (int i = 0; i < lenstrings; i++) {
        char *s = strings[i];
        printf("----- %d ------\n", i + 1);
        RespObj *o = resp_parse(&s, &status, &ar);
        printf("status: %d\n", status);

        if (status == 0 && o != NULL) {
            resp_prettyprint(o, stdout);

            printf("---\n matching marshal... ");
            char *a = resp_marshal(o, &size);
            int r = strcmp(a, strings[i]);
            printf("%d\n", r);
        }
    }

    arena_free(&ar);

    return 0;
}