# GBA Multiboot CLI

CLI tool for uploading GBA multiboot ROMs via a Raspberry Pi Pico serial bridge.

See the [main repository](https://github.com/loopj/gba-multiboot-pico) for firmware installation and wiring instructions.

## Installation

```bash
pipx install multiboot
```

Or with pip:

```bash
pip install multiboot
```

## Usage

Upload a ROM, auto-detecting the Pico device:

```bash
multiboot upload rom.gba
```

Specify a serial port manually:

```bash
multiboot upload rom.gba --port /dev/ttyUSB0
```

Set a custom connection timeout:

```bash
multiboot upload rom.gba --timeout 20
```

Print the GBA's serial output after the upload:

```bash
multiboot upload rom.gba --monitor
```

Print the serial output of a program that is already running:

```bash
multiboot monitor
```
