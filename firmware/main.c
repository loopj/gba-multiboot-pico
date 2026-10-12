/**
 * Minimal USB serial bridge to the GBA link port, for multiboot and for talking to a running program.
 *
 * Every message from the host starts with a command byte:
 *
 *   'U'          UART mode: forward the GBA's UART output from SO to USB serial, and allow sending to its SI.
 *   'S' len data Send len bytes to the GBA over UART, in UART mode.
 *   'N'          Normal mode: drive the link port as SPI, the 32-bit mode multiboot uses.
 *   'X' word     Exchange a little-endian 32-bit word over SPI in normal mode, returning the GBA's reply.
 *   'V'          Reply with the protocol version byte.
 *
 * The letters follow GBATEK's names for the link port modes, and M, G and J are kept for its multiplay, general
 * purpose and JOY BUS modes. SPI words are sent big-endian. The bridge starts in UART mode.
 */
#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

#define SPI_BAUD  100000
#define SPI_PORT  SPI_INSTANCE(PICO_DEFAULT_SPI)
#define UART_BAUD 115200

#define PROTOCOL_VERSION 2

static PIO uart_pio = pio0;
static uint rx_sm;
static uint tx_sm;
static uint rx_offset;
static uint tx_offset;
static bool uart_mode;

// Read the next byte from the host, waiting for it
static uint8_t read_host_byte(void)
{
  int c;
  do
    c = getchar_timeout_us(1000);
  while (c < 0);

  return (uint8_t)c;
}

// Hand SO and SI to the UART programs, leaving SC undriven
static void enter_uart_mode(void)
{
  if (uart_mode)
    return;

  uart_rx_program_init(uart_pio, rx_sm, rx_offset, PICO_DEFAULT_SPI_RX_PIN, UART_BAUD);
  uart_tx_program_init(uart_pio, tx_sm, tx_offset, PICO_DEFAULT_SPI_TX_PIN, UART_BAUD);
  gpio_init(PICO_DEFAULT_SPI_SCK_PIN);
  gpio_pull_up(PICO_DEFAULT_SPI_SCK_PIN);
  uart_mode = true;
}

// Give the link port back to SPI
static void enter_normal_mode(void)
{
  if (!uart_mode)
    return;

  pio_sm_set_enabled(uart_pio, rx_sm, false);
  pio_sm_set_enabled(uart_pio, tx_sm, false);
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  uart_mode = false;
}

// Exchange one 32-bit word with the GBA over SPI
static void exchange_word(void)
{
  uint8_t command[4];
  uint8_t response[4];

  // Read the word from the host, converting from little-endian to big-endian
  for (size_t i = 0; i < 4; i++)
    command[3 - i] = read_host_byte();

  // Send it and read the reply, ignoring words sent outside normal mode
  if (uart_mode)
    return;
  spi_write_read_blocking(SPI_PORT, command, response, 4);

  // Send the reply back, converting from big-endian to little-endian
  for (size_t i = 0; i < 4; i++)
    putchar_raw(response[3 - i]);
}

// Send bytes from the host to the GBA over UART
static void send_bytes(void)
{
  uint8_t len = read_host_byte();
  for (uint8_t i = 0; i < len; i++) {
    uint8_t c = read_host_byte();
    if (uart_mode)
      uart_tx_program_putc(uart_pio, tx_sm, (char)c);
  }
}

int main()
{
  // Initializes stdio over USB
  stdio_init_all();

  // Init SPI
  spi_init(SPI_PORT, SPI_BAUD);
  spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

  // Load the UART programs, then start in UART mode
  rx_sm     = pio_claim_unused_sm(uart_pio, true);
  tx_sm     = pio_claim_unused_sm(uart_pio, true);
  rx_offset = pio_add_program(uart_pio, &uart_rx_program);
  tx_offset = pio_add_program(uart_pio, &uart_tx_program);
  uart_mode = false;
  enter_uart_mode();

  while (true) {
    // Handle a command from the host
    int c = getchar_timeout_us(0);
    if (c == 'U') {
      enter_uart_mode();
    } else if (c == 'N') {
      enter_normal_mode();
    } else if (c == 'X') {
      exchange_word();
    } else if (c == 'S') {
      send_bytes();
    } else if (c == 'V') {
      putchar_raw(PROTOCOL_VERSION);
    }

    // Forward GBA UART output to USB serial
    if (uart_mode && !pio_sm_is_rx_fifo_empty(uart_pio, rx_sm))
      putchar_raw(uart_rx_program_getc(uart_pio, rx_sm));
  }

  return 0;
}
