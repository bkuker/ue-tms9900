
static volatile unsigned short *M0STAT = (void*)0xF000;
static volatile unsigned short *M0RX = (void*)0xF002;
static volatile unsigned short *M0TX = (void*)0xF004;
static volatile unsigned short *M0RST = (void*)0xF006;

static void putchar(char c)
{
    while ( *M0STAT & 0x01 );
    *M0TX = c;
}

static void print(const char *s)
{
    while (*s) {
        putchar(*s++);
    }
}

void main(){
    print("Hello C Compiler!\r\nType Something!\r\n");
    char buf[20];
    int i = 0;
    while(1){
        if ( *M0STAT & 0x02 ){
            char in = *M0RX;
            *M0RST = 0;

            if ( in == 0x0d ){
                buf[i] = 0;
                print("\r\n You typed: ");
                print(buf);
                print("\r\n");
                i = 0;
            } else {
                buf[i++] = in;
            }
            putchar(*M0RX);

        }
    }
}
