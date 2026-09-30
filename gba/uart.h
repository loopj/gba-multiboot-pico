/**
 * Debug output over the GBA link port in UART mode, for reading with `multiboot --monitor`.
 */
#pragma once

/**
 * Put the link port in UART mode at 115200 baud, 8 data bits, no parity, send only.
 */
void uart_init(void);

/**
 * Send one byte, blocking while the send buffer is full.
 * @param c Byte to send.
 */
void uart_putc(char c);

/**
 * Send a NUL-terminated string, blocking until the last byte is queued.
 * @param s String to send.
 */
void uart_puts(const char *s);

/**
 * Format and send a string, blocking until the last byte is queued.
 *
 * Output longer than 127 bytes is truncated. Floating point conversions are not supported.
 * @param fmt printf-style format string.
 */
void uart_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
