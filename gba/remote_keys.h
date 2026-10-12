/**
 * GBA buttons held remotely by the host, such as with `multiboot keys`, received over the link port's UART.
 */
#pragma once

#include <stdint.h>

/**
 * Read the key states the host has sent and return the buttons it holds. Call it once a frame after uart_init, and
 * combine the result with the buttons pressed on the GBA.
 *
 * Reads every byte the UART has received, so a program using remote keys can't receive other data at the same time.
 * @return Held buttons in the bit order of the KEYINPUT register, with a bit set for each held button.
 */
uint16_t remote_keys_held(void);
