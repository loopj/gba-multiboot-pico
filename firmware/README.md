# USB Serial Bridge Firmware for the GBA Link Port

The firmware uploads multiboot images over the link port's SPI mode, and talks to a running program over its UART mode.

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

## Protocol

Every message from the host starts with a command byte. The bridge starts in UART mode. Command bytes `M`, `G` and `J` are reserved for potential future multiplay, general purpose and Joybus modes.

| Command | Payload | Reply | What it does |
|---------|---------|-------|--------------|
| `U` | none | none | UART mode. The GBA's UART output is forwarded to the host, and `S` can send to the GBA. |
| `S` | length byte, then that many bytes | none | Sends the bytes to the GBA over UART, in UART mode. |
| `N` | none | none | Normal mode, 32-bit, which multiboot uses. The link port becomes SPI. |
| `X` | 32-bit word, little-endian | 32-bit word, little-endian | Exchanges one word with the GBA over SPI, in normal mode. |
| `V` | none | one byte | Replies with the protocol version, currently 2. |

The link port runs at 115200 baud, 8 data bits, no parity, in UART mode, and SPI words go out big-endian at 100 kHz in normal mode.
