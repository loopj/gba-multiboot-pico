# GBA Multiboot Loader for Raspberry Pi Pico

Firmware and CLI for uploading GBA multiboot ROMs and monitoring GBA serial output using a Raspberry Pi Pico or other RP2040-based board.

## Usage

Flash your pico with the multiboot firmware, see the [latest release](https://github.com/loopj/gba-multiboot-pico/releases/latest) for prebuilt .uf2 files for common RP2040 boards.

Install the multiboot CLI:

```bash
pipx install multiboot
```

The CLI checks that the firmware speaks the same protocol version, so flash the matching firmware whenever you upgrade the CLI.

Run the multiboot CLI, specifying the path to your GBA multiboot ROM:

```bash
multiboot upload path/to/your/game.mb
```

You can also specify the serial port and timeout if needed:

```bash
multiboot upload path/to/your/game.mb --port /dev/ttyUSB0 --timeout 20
```

## Serial monitor

Add `--monitor` to monitor the GBA's serial output after the upload. Press Ctrl-C to exit.

```bash
multiboot upload path/to/your/game.mb --monitor
```

To watch a program that is already running, such as one started from a flash cart, use `monitor` on its own.

```bash
multiboot monitor
```

On the GBA, put the link port in [UART mode](https://problemkaputt.de/gbatek.htm#siouartmode) at 115200 baud with 8 data bits and no parity.

The [gba](gba) directory has a small library that does this. Copy `uart.h` and `uart.c` into your project.

```c
#include "uart.h"

int main(void)
{
  uart_init();
  uart_printf("Hello from the GBA, frame %d\n", 0);
  ...
}
```

## Remote keys

`keys` presses GBA buttons from your computer while a program runs, and prints the program's serial output meanwhile. The keyboard follows mGBA's default bindings, with the arrow keys for the D-pad, X for A, Z for B, A for L, S for R, Enter for Start and Backspace for Select. Press Ctrl-C to exit.

```bash
multiboot keys
```

A terminal only reports key presses, so each press holds its button for 0.15 seconds, or as long as `--hold` gives. For tests, `--script` runs steps separated by semicolons instead. `tap` presses buttons for the hold time, `hold` presses them for a number of seconds, and `wait` pauses. Join buttons held together with a plus sign.

```bash
multiboot keys --script "tap A; wait 0.5; hold RIGHT+L 0.3; tap START"
```

On the GBA, copy `remote_keys.h` and `remote_keys.c` from the [gba](gba) directory alongside the UART files. Call `remote_keys_held()` once a frame after `uart_init()` and combine its buttons with the ones pressed on the GBA. It returns the held buttons in the KEYINPUT bit order, with a bit set for each held button, and it reads every byte the UART receives.

```c
#include "remote_keys.h"
#include "uart.h"

uint16_t pressed = (~REG_KEYINPUT & 0x3FF) | remote_keys_held();
```

The host sends each change as two bytes, the first with its top bit set and buttons 0 to 6, the second with buttons 7 to 9.

## Wiring

You'll need a sacrificial link cable, I recommend using a Gameboy Color link cable since the aftermarket versions typically have all pins connected.

Aftermarket cables all have wildly different internal wire colors, so you'll need to use a multimeter to figure out which wire goes to which pin.

The pinout for the male connector is as follows (when looking at the connector):

```
                             ___
   _____________        ____|   |____
 /               \    /               \
 |  VCC  SI  SC  |    |  VCC  SI  SC  |
 |  SO   SD  GND |    |  SO   SD  GND |
 |_______________|    \_______________/
     GBC Cable            GBA Cable
```

The firmware uses the default SPI peripheral and pins for the configured board. For the Raspberry Pi Pico and Seeed XIAO RP2040, the pin mapping is as follows:

| GBA Cable | Raspberry Pi Pico | Raspberry Pi Pico 2 | Seeed XIAO RP2040 |
|-----------|-------------------|---------------------|-------------------|
| GND       | GND               | GND                 | GND               |
| SC        | GPIO18            | GPIO18              | GPIO2             |
| SI        | GPIO19            | GPIO19              | GPIO3             |
| SO        | GPIO16            | GPIO16              | GPIO4             |


Check the [Pico SDK boards files](https://github.com/raspberrypi/pico-sdk/tree/master/src/boards/include/boards) for the pin mappings for your board.

## License

This project is licensed under the MIT License. See [cli/LICENSE](cli/LICENSE) and [firmware/LICENSE](firmware/LICENSE) for more details.

Upload CLI was originally based on jojolebarjos' excellent [gba-multiboot](https://github.com/jojolebarjos/gba-multiboot) upload script.
