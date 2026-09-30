// Expose newlib's vsniprintf under strict -std modes
#define _DEFAULT_SOURCE

#include "uart.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// Link port registers
#define UART_RCNT     (*(volatile uint16_t *)0x04000134)
#define UART_SIOCNT   (*(volatile uint16_t *)0x04000128)
#define UART_SIODATA8 (*(volatile uint16_t *)0x0400012A)

// SIOCNT bits in UART mode, per GBATEK
#define UART_115200    0x0003
#define UART_SEND_FULL 0x0010
#define UART_8BIT      0x0080
#define UART_SEND_EN   0x0400
#define UART_MODE      0x3000

void uart_init(void)
{
  // Select the serial modes, then UART mode with only the transmitter enabled
  UART_RCNT   = 0;
  UART_SIOCNT = 0;
  UART_SIOCNT = UART_MODE | UART_8BIT | UART_SEND_EN | UART_115200;
}

void uart_putc(char c)
{
  while (UART_SIOCNT & UART_SEND_FULL)
    ;
  UART_SIODATA8 = (uint8_t)c;
}

void uart_puts(const char *s)
{
  while (*s)
    uart_putc(*s++);
}

void uart_printf(const char *fmt, ...)
{
  char buf[128];
  va_list args;

  // Integer-only formatting keeps newlib's float printf out of the ROM
  va_start(args, fmt);
  vsniprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  uart_puts(buf);
}
