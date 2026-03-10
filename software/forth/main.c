#include "zforth.h"

#define yield __asm__("BLWP @>10")

extern char coreZF[];

static void putchar(zf_ctx *ctx, char c)
{
    while ( *ctx->stat & 0x01 )
		yield;

    *(ctx->mux+2) = c;

	*ctx->stat = *ctx->mux;
}

static char getchar(zf_ctx *ctx){
    while(1){
        if ( *ctx->stat & 0x02 ){        
			*ctx->stat = *ctx->mux;    
            char in = *(ctx->mux+1);
            *(ctx->mux+3) = 0;
            return in;
        }
		yield;
    }
}

static void puts(zf_ctx *ctx, const char *s)
{
    while (*s)
        putchar(ctx, *s++);
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
	zf_cell len;
	zf_cell addr;
	switch((int)id) {

		case ZF_SYSCALL_EMIT:
			putchar(ctx, (char)zf_pop(ctx));
			break;

		case ZF_SYSCALL_PRINT:
			puts(ctx, itoa_small(zf_pop(ctx), buf, 15));
			putchar(ctx, ' ');
			break;

		case ZF_SYSCALL_TELL:
			len = zf_pop(ctx);
			addr = zf_pop(ctx);
			while ( len-- ){
				putchar(ctx, *(ctx->dict + addr++) );
			}
			break;
	}

	return 0;
}


zf_cell zf_host_parse_num(zf_ctx *ctx, const char *buf)
{
	int ret = 0;
	int s = 1;
	if ( *buf == '-' ){
		s = -1;
		buf++;
	}
	while (*buf){
		if ( *buf < '0' || *buf > '9' )
			zf_abort(ctx, ZF_ABORT_NOT_A_WORD);
		ret = ret * 10;
	    ret += *buf - '0';
		buf = buf + 1;
	}
	return ret * s;
}


static char* e = "okieomdudoruronwcoisdziuex";
void printErrorCode(zf_ctx *ctx, zf_result r){
	putchar(ctx, 'E');
	putchar(ctx, *(e + r*2) );
	putchar(ctx, *(1 + e + r*2) );
}


void main(unsigned short *mux, unsigned short *stat){
    char buf[16];
	zf_ctx _ctx;
	zf_ctx *ctx = &_ctx;

	/* Initialize zforth */
	zf_init(ctx, 1);

	ctx->mux = mux;
	ctx->stat = stat;
	putchar(ctx, 'z');


	putchar(ctx, 'F');
	zf_bootstrap(ctx);
	putchar(ctx, 'o');

	zf_result r = zf_eval(ctx, coreZF);

	if(r != ZF_OK)
		printErrorCode(ctx, r);

	puts(ctx, "rth.\r\n");
	/* Main loop: read words and eval */

	uint8_t l = 0;

	for(;;) {
		int c = getchar(ctx);
		putchar(ctx, c);
		if ( c == 13 )
			putchar(ctx, 10);
		if(c == 10 || c == 13 || c == 32) {
			zf_result r = zf_eval(ctx, buf);
			if(r != ZF_OK)
				printErrorCode(ctx, r);
			l = 0;
		} else if(l < sizeof(buf)-1) {
			buf[l++] = c;
		}

		buf[l] = '\0';
	}
}

