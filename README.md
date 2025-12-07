# GBA Multiboot Loader for Raspberry Pi Pico

Minimal USB Serial to SPI passthrough bridge for GBA multiboot, allowing you to upload GBA multiboot ROMs using a Raspberry Pi Pico or other RP2040-based board.

Takes little-endian 32-bit commands over USB serial, converts to the SPI format expected by the GBA multiboot protocol, and returns little-endian 32-bit responses.

Compatible with jojolebarjos' [gba-multiboot](https://github.com/jojolebarjos/gba-multiboot) upload script.

## Usage

Flash your pico with the multiboot firmware, see the [latest release](https://github.com/loopj/gba-multiboot-pico/releases/latest) for prebuilt .uf2 files for common RP2040 boards.

Download [`upload.py`](https://github.com/jojolebarjos/gba-multiboot/blob/master/serial/upload.py) from jojolebarjos' gba-multiboot repository.

Run the upload script, specifying the path to your GBA multiboot ROM and the serial port of your pico device:

```bash
python upload.py path/to/your/game.mb /dev/ttyACM0 
```

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