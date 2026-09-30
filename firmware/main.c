/**
 * Minimal USB Serial to SPI passthrough bridge, configured for GBA multiboot.
 *
 * Expects little-endian 32-bit commands over USB serial and returns little-endian
 * 32-bit responses.
 *
 * Converts to big-endian for SPI communication.
 *
 * While the host is idle, forwards GBA UART output from SO to USB serial.
 */
#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "uart_rx.pio.h"

#define SPI_BAUD 100000
#define SPI_PORT SPI_INSTANCE(PICO_DEFAULT_SPI)
#define UART_BAUD    115200
#define UART_IDLE_US 250000

static PIO uart_pio = pio0;
static uint uart_sm;
static uint uart_offset;

static uint8_t read_host_byte(void)
{
  bool monitoring     = false;
  uint64_t idle_since = time_us_64();

  while (true) {
    // Return SO to SPI as soon as the host sends a byte
    int c = getchar_timeout_us(0);
    if (c >= 0) {
      if (monitoring) {
        pio_sm_set_enabled(uart_pio, uart_sm, false);
        gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
      }
      return (uint8_t)c;
    }

    // Listen for GBA UART output once the host has been idle for a while
    if (!monitoring && time_us_64() - idle_since > UART_IDLE_US) {
      uart_rx_program_init(uart_pio, uart_sm, uart_offset, PICO_DEFAULT_SPI_RX_PIN, UART_BAUD);
      monitoring = true;
    }

    // Forward GBA UART output to USB serial
    if (monitoring && !pio_sm_is_rx_fifo_empty(uart_pio, uart_sm))
      putchar_raw(uart_rx_program_getc(uart_pio, uart_sm));
  }
}

int main()
{
  // Initializes stdio over USB
  stdio_init_all();

  // Init SPI
  spi_init(SPI_PORT, SPI_BAUD);
  spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

  // Configure SPI pins
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);

  // Load the UART receiver used while the host is idle
  uart_sm     = pio_claim_unused_sm(uart_pio, true);
  uart_offset = pio_add_program(uart_pio, &uart_rx_program);

  // Buffers for command and response
  uint8_t command[4];
  uint8_t response[4];

  while (true) {
    // Read command from USB serial, converting from little-endian to big-endian
    for (size_t i = 0; i < 4; i++)
      command[3 - i] = read_host_byte();

    // Send command and read response via SPI
    spi_write_read_blocking(SPI_PORT, command, response, 4);

    // Send response back over USB serial, converting from big-endian to little-endian
    for (size_t i = 0; i < 4; i++)
      putchar_raw(response[3 - i]);
  }

  return 0;
}
