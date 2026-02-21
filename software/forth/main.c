#include "zforth.h"

#define yield __asm__("BLWP @>4")

static void putchar(volatile unsigned short *mux, char c)
{
    while ( *mux & 0x01 )
		yield;

    *(mux+2) = c;
}

static char getchar(volatile unsigned short *mux){
    while(1){
        if ( *mux & 0x02 ){            
            char in = *(mux+1);
            *(mux+3) = 0;
            return in;
        }
		yield;
    }
}

static void puts(unsigned short *mux, const char *s)
{
    while (*s)
        putchar(mux, *s++);
    putchar(mux, '\r');
    putchar(mux, '\n');
}

// Writes to buffer starting at endPos, returns pointer to start
char* itoa_small(int val, char* buf, int endPos) {
    int i = endPos;
    buf[i--] = '\0';
    if (val == 0) { buf[i] = '0'; return &buf[i]; }
    
    int neg = val < 0;
    if (neg) val = -val;
    
    while (val > 0) {
        buf[i--] = (val % 10) + '0';
        val /= 10;
    }
    if (neg) buf[i--] = '-';
    return &buf[i + 1];
}

zf_input_state zf_host_sys(zf_ctx *ctx, zf_syscall_id id, const char *input)
{
	char buf[16];

	switch((int)id) {

		case ZF_SYSCALL_EMIT:
			putchar(ctx->mux, (char)zf_pop(ctx));
			break;

		case ZF_SYSCALL_PRINT:
			puts(ctx->mux, itoa_small(zf_pop(ctx), buf, 15));
			break;
	}

	return 0;
}


zf_cell zf_host_parse_num(zf_ctx *ctx, const char *buf)
{
	int ret = 0;
	while (*buf){
		if ( *buf < '0' || *buf > '9' )
			zf_abort(ctx, ZF_ABORT_NOT_A_WORD);
		ret = ret * 10;
	    ret += *buf - '0';
		buf = buf + 1;
	}
	return ret;
}


void main(unsigned short *mux){
    char buf[16];
	zf_ctx _ctx;
	zf_ctx *ctx = &_ctx;

	/* Initialize zforth */

	putchar(mux, 'z');
	zf_init(ctx, 1);

	ctx->mux = mux;

	putchar(mux, 'F');
	zf_bootstrap(ctx);
	putchar(mux, 'o');
	zf_eval(ctx, ": . 1 sys ;");

	puts(mux, "rth.");
	/* Main loop: read words and eval */

	uint8_t l = 0;

	for(;;) {
		int c = getchar(mux);
		putchar(mux, c);
		if ( c == 13 )
			putchar(mux, 10);
		if(c == 10 || c == 13 || c == 32) {
			zf_result r = zf_eval(ctx, buf);
			if(r != ZF_OK) puts(mux, "A");
			l = 0;
		} else if(l < sizeof(buf)-1) {
			buf[l++] = c;
		}

		buf[l] = '\0';
	}
}

