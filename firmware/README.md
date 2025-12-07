# USB Serial to SPI passthrough firmware for GBA multiboot

## Building the firmware

```bash
PICO_BOARD=pico cmake -Bbuild && cmake --build build
```

Replace `pico` with your board name if you're using a different RP2040 board, e.g. `seeed_xiao_rp2040`.

## Flashing the firmware

Enter bootloader mode by holding the BOOTSEL button while plugging in the Pico. Then copy the generated `build/app.uf2` file to the RPI-RP2 drive that appears, or use `picotool`:

```bash
picotool load -f build/app.uf2
```