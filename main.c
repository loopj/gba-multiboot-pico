/**
 * Minimal USB Serial to SPI passthrough bridge, configured for GBA multiboot.
 *
 * Expects little-endian 32-bit commands over USB serial and returns little-endian
 * 32-bit responses.
 *
 * Converts to big-endian for SPI communication.
 */
#include <stdio.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"

#define SPI_BAUD 100000
#define SPI_PORT SPI_INSTANCE(PICO_DEFAULT_SPI)

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

  // Buffers for command and response
  uint8_t command[4];
  uint8_t response[4];

  while (true) {
    // Read command from USB serial, converting from little-endian to big-endian
    for (size_t i = 0; i < 4; i++)
      command[3 - i] = (uint8_t)getchar();

    // Send command and read response via SPI
    spi_write_read_blocking(SPI_PORT, command, response, 4);

    // Send response back over USB serial, converting from big-endian to little-endian
    for (size_t i = 0; i < 4; i++)
      putchar_raw(response[3 - i]);
  }

  return 0;
}