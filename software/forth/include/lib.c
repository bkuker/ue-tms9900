#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>

unsigned short strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

int memcmp(const void *a, const void *b, unsigned short n) {
    const unsigned char *p = a;
    const unsigned char *q = b;
    while (n--) {
        if (*p != *q)
            return *p - *q;
        p++; q++;
    }
    return 0;
}

int isspace(int c) {
    return c==' ' || c=='\t' || c=='\n' ||
           c=='\r' || c=='\f' || c=='\v';
}