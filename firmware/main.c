/*
 * DAC4813 firmware for Arduino Uno Rev. 3 (ATmega328P @ 16 MHz)
 *
 * Data bus (12-bit parallel):
 *   PD2→D3  PD3→D2  PD4→D1  PD5→D0  PD6→D11  PD7→D10
 *   PB0→D9  PB1→D8  PB2→D7  PB3→D6  PB4→D5   PB5→D4
 *
 * Control signals (PORTC, active-low):
 *   PC0=/WR  PC1=/EN1  PC2=/EN2  PC3=/EN3  PC4=/EN4  PC5=/LDAC
 *
 * Serial protocol (115200 8N1, \n-terminated):
 *   "<ch> <hex>\n"  → set channel ch (1-4) to 12-bit hex value, echo on success
 *   "*RST\n"        → reset all channels to 0x800 (0V), echo "*RST"
 *   "*IDN?\n"       → reply "DAC4813 on Arduino Uno Rev. 3"
 *   (invalid)       → reply "ERR"
 */

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BAUD     115200UL
#define UBRR_VAL ((F_CPU / (8UL * BAUD)) - 1)  /* U2X mode */

/* ── UART ──────────────────────────────────────────────────────────────── */

static void uart_init(void)
{
    UBRR0  = UBRR_VAL;
    UCSR0A = (1 << U2X0);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  /* 8N1 */
}

static void uart_putchar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

static char uart_getchar(void)
{
    while (!(UCSR0A & (1 << RXC0)));
    return (char)UDR0;
}

static void uart_puts(const char *s)
{
    while (*s)
        uart_putchar(*s++);
    uart_putchar('\n');
}

/* ── DAC hardware ──────────────────────────────────────────────────────── */

static void dac_init(void)
{
    /* PD2-PD7 outputs, PD0/PD1 kept for UART */
    DDRD  |= 0xFC;
    /* PB0-PB5 outputs */
    DDRB  |= 0x3F;
    /* PC0-PC5 outputs, idle HIGH */
    DDRC  |= 0x3F;
    PORTC |= 0x3F;
}

/*
 * Scatter a 12-bit value across PORTD[7:2] and PORTB[5:0].
 *
 * PORTD bits:  PD6=D11  PD7=D10  PD2=D3  PD3=D2  PD4=D1  PD5=D0
 * PORTB bits:  PB0=D9   PB1=D8   PB2=D7  PB3=D6  PB4=D5  PB5=D4
 */
static void dac_set_data(uint16_t v)
{
    uint8_t pd = PORTD & 0x03;                /* preserve PD0/PD1 (UART) */
    pd |= (uint8_t)(((v >> 11) & 1u) << 6);  /* D11 → PD6 */
    pd |= (uint8_t)(((v >> 10) & 1u) << 7);  /* D10 → PD7 */
    pd |= (uint8_t)(((v >>  3) & 1u) << 2);  /* D3  → PD2 */
    pd |= (uint8_t)(((v >>  2) & 1u) << 3);  /* D2  → PD3 */
    pd |= (uint8_t)(((v >>  1) & 1u) << 4);  /* D1  → PD4 */
    pd |= (uint8_t)(((v >>  0) & 1u) << 5);  /* D0  → PD5 */
    PORTD = pd;

    uint8_t pb = 0;
    pb |= (uint8_t)(((v >> 9) & 1u) << 0);   /* D9 → PB0 */
    pb |= (uint8_t)(((v >> 8) & 1u) << 1);   /* D8 → PB1 */
    pb |= (uint8_t)(((v >> 7) & 1u) << 2);   /* D7 → PB2 */
    pb |= (uint8_t)(((v >> 6) & 1u) << 3);   /* D6 → PB3 */
    pb |= (uint8_t)(((v >> 5) & 1u) << 4);   /* D5 → PB4 */
    pb |= (uint8_t)(((v >> 4) & 1u) << 5);   /* D4 → PB5 */
    PORTB = pb;
}

/*
 * Write value (0x000..0xFFF) to DAC channel (1..4).
 *
 * Sequence: present data → assert /ENch → assert /WR+/LDAC → deassert all.
 */
static void dac_write(uint8_t ch, uint16_t value)
{
    dac_set_data(value);
    PORTC &= (uint8_t)~(1u << ch);                    /* assert /ENch        */
    PORTC &= (uint8_t)~((1u << 0) | (1u << 5));       /* assert /WR + /LDAC  */
    _delay_us(1);
    PORTC |= (uint8_t)((1u << 0) | (1u << ch) | (1u << 5)); /* deassert all */
}

static void dac_reset_all(void)
{
    for (uint8_t ch = 1; ch <= 4; ch++)
        dac_write(ch, 0x800);
}

/* ── Command parser ────────────────────────────────────────────────────── */

#define BUF_LEN 16

static void process_command(char *buf)
{
    /* *IDN? */
    if (strcmp(buf, "*IDN?") == 0) {
        uart_puts("DAC4813 on Arduino Uno Rev. 3");
        return;
    }

    /* *RST */
    if (strcmp(buf, "*RST") == 0) {
        dac_reset_all();
        uart_puts("*RST");
        return;
    }

    /* "<ch> <hex>" — channel digit, space, 1-3 hex digits */
    if (buf[0] >= '1' && buf[0] <= '4' && buf[1] == ' ') {
        uint8_t ch = (uint8_t)(buf[0] - '0');
        char   *hex = buf + 2;

        /* validate: 1-3 hex characters only */
        uint8_t len = 0;
        for (char *p = hex; *p; p++, len++) {
            if (!isxdigit((unsigned char)*p))
                goto err;
        }
        if (len == 0 || len > 3)
            goto err;

        uint16_t value = (uint16_t)strtol(hex, NULL, 16);
        if (value > 0xFFF)
            goto err;

        dac_write(ch, value);
        uart_putchar(buf[0]);
        uart_putchar(' ');
        uart_puts(hex);
        return;
    }

err:
    uart_puts("ERR");
}

/* ── Main ──────────────────────────────────────────────────────────────── */

int main(void)
{
    uart_init();
    dac_init();
    dac_reset_all();

    char    buf[BUF_LEN];
    uint8_t pos = 0;

    for (;;) {
        char c = uart_getchar();

        if (c == '\r')
            continue;  /* ignore CR in CRLF sequences */

        if (c == '\n') {
            buf[pos] = '\0';
            if (pos > 0)
                process_command(buf);
            pos = 0;
            continue;
        }

        if (pos < BUF_LEN - 1)
            buf[pos++] = c;
        /* silently drop characters that overflow the buffer */
    }
}
