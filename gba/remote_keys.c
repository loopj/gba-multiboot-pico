#include "remote_keys.h"

#include "uart.h"

// A key state arrives as two bytes, the first with its top bit set and buttons 0 to 6, the second with buttons 7 to 9
#define REMOTE_KEYS_FIRST 0x80

uint16_t remote_keys_held(void)
{
  static uint16_t held;
  static int first = -1;

  // Pair each first byte with the second byte after it, dropping a second byte with no first before it
  for (int c = uart_getc(); c >= 0; c = uart_getc()) {
    if (c & REMOTE_KEYS_FIRST) {
      first = c & 0x7F;
    } else if (first >= 0) {
      held  = (uint16_t)(first | (c & 0x07) << 7);
      first = -1;
    }
  }

  return held;
}
