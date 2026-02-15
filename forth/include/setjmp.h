#ifndef SETJMP_H
#define SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned short regs[16];  /* workspace registers R0–R15 */
    unsigned short wp;        /* workspace pointer */
    unsigned short pc;        /* return address */
} jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif

#endif