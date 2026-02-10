/*
 * ZX0 Decompressor
 * Based on reference implementation by Einar Saukas
 * https://github.com/einar-saukas/ZX0
 * 
 * Simplified for embedded systems: decompresses from ROM to RAM
 * Usage: Call zx0_decompress(src, dst) where src points to compressed data
 *        and dst points to destination buffer (must be large enough)
 */
#define ASSUME(x) do { if (!(x)) for(;;); } while (0)
#define INITIAL_OFFSET 1

static char z[] = {0x68, 0x48, 0x65, 0x6C, 0x1E, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x2C, 0x20, 0xE6, 0x68, 77, 0x2D, 0x21, 0x55, 0x56};
static char o[256];

// Global state for decompression
static const unsigned char *input_ptr;
static unsigned char *output_ptr;
static int bit_mask;
static int bit_value;
static int backtrack;
static int last_byte;

static int read_bit(void) {
    if (backtrack) {
        backtrack = 0;
        return last_byte & 1;
    }
    bit_mask >>= 1;
    if (bit_mask == 0) {
        bit_mask = 128;
        bit_value = *input_ptr++;  // inlined read_byte
    }
    return (bit_value & bit_mask) ? 1 : 0;
}

static int read_interlaced_elias_gamma(int inverted) {
    int value = 1;
    while (!read_bit()) {
        value = (value << 1) | (read_bit() ^ inverted);
    }
    return value;
}

static void write_bytes(int offset, int length) {
    while (length-- > 0) {
        *output_ptr = output_ptr[-offset];  // read from offset back
        output_ptr++;                        // then increment
    }
}

/*
 * Decompress ZX0 data
 * src: pointer to compressed data
 * dst: pointer to destination buffer (must be large enough)
 * Returns: pointer to byte after last decompressed byte
 */
unsigned char *zx0_decompress(const unsigned char *src, unsigned char *dst) {
    int last_offset = INITIAL_OFFSET;
    int length;
    int i;

    // Initialize state
    input_ptr = src;
    output_ptr = dst;
    bit_mask = 0;
    backtrack = 0;

COPY_LITERALS:
    length = read_interlaced_elias_gamma(0);
    for (i = 0; i < length; i++)
        *output_ptr++ = *input_ptr++;  // inlined write_byte and read_byte
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;

/*COPY_FROM_LAST_OFFSET:*/
    length = read_interlaced_elias_gamma(0);
    write_bytes(last_offset, length);
    if (!read_bit())
        goto COPY_LITERALS;

COPY_FROM_NEW_OFFSET:
    last_offset = read_interlaced_elias_gamma(1);  // new format only
    if (last_offset == 256) {
        return output_ptr;  // EOF - return pointer past last byte
    }
    last_byte = *input_ptr++;  // inlined read_byte
    last_offset = last_offset * 128 - (last_byte >> 1);
    backtrack = 1;
    length = read_interlaced_elias_gamma(0) + 1;
    write_bytes(last_offset, length);
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;
    else
        goto COPY_LITERALS;
}


static volatile unsigned short *M0STAT = (void*)0xF000;
static volatile unsigned short *M0RX = (void*)0xF002;
static volatile unsigned short *M0TX = (void*)0xF004;
static volatile unsigned short *M0RST = (void*)0xF006;

int main(void) {

    zx0_decompress(z, o);

    char*s = o;
    while (*s) {
        while ( *M0STAT & 0x01 );
         *M0TX = *s++;
    }
}

