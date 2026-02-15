#include "zforth.h"

static volatile unsigned short *M0STAT = (void*)0xF000;
static volatile unsigned short *M0RX = (void*)0xF002;
static volatile unsigned short *M0TX = (void*)0xF004;
static volatile unsigned short *M0RST = (void*)0xF006;

static void putchar(char c)
{
    while ( *M0STAT & 0x01 );
    *M0TX = c;
}

static char getchar(){
    while(1){
        if ( *M0STAT & 0x02 ){            
            char in = *M0RX;
            *M0RST = 0;
            return in;
        }
    }
}

static void puts(const char *s)
{
    while (*s)
        putchar(*s++);
    putchar('\r');
    putchar('\n');
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
			putchar((char)zf_pop(ctx));
			break;

		case ZF_SYSCALL_PRINT:
			puts(itoa_small(zf_pop(ctx), buf, 16));
			break;
	}

	return 0;
}

void zf_host_trace(zf_ctx *ctx, const char *fmt, va_list va)
{
	puts(fmt);
}

zf_cell zf_host_parse_num(zf_ctx *ctx, const char *buf)
{
    return *buf - '0';
}


void main(){
    char buf[32];
	zf_ctx _ctx;
	zf_ctx *ctx = &_ctx;

	/* Initialize zforth */

	puts("init...");
	zf_init(ctx, 1);
	puts("bootstrap...");
	zf_bootstrap(ctx);
	puts("eval...");
	zf_eval(ctx, ": . 1 sys ;");

	puts("zForth ready!");
	/* Main loop: read words and eval */

	uint8_t l = 0;

	for(;;) {
		int c = getchar();
		putchar(c);
		if ( c == 13 )
			putchar(10);
		if(c == 10 || c == 13 ) {
			zf_result r = zf_eval(ctx, buf);
			if(r != ZF_OK) puts("A");
			l = 0;
		} else if(l < sizeof(buf)-1) {
			buf[l++] = c;
		}

		buf[l] = '\0';
	}
}

