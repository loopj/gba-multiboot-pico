"""GBA multiboot ROM uploader CLI.

Based on upload.py from https://github.com/jojolebarjos/gba-multiboot
See LICENSE for original license details.
"""

import argparse
import sys
import time

import serial
import serial.tools.list_ports
from rich.console import Console
from rich.progress import (
    BarColumn,
    Progress,
    SpinnerColumn,
    TaskProgressColumn,
    TextColumn,
)

console = Console()


PICO_VID = 0x2E8A
PICO_PID = 0x000A


def xfer32(port, output):
    """Exchange 32 bit payloads with the GBA attached to the given serial port."""
    # Send bytes
    output_bytes = bytes(
        (
            output & 0xFF,
            (output >> 8) & 0xFF,
            (output >> 16) & 0xFF,
            (output >> 24) & 0xFF,
        )
    )
    port.write(output_bytes)
    port.flush()

    # Get back the reply
    input_bytes = port.read(4)
    input = (
        input_bytes[0]
        | input_bytes[1] << 8
        | input_bytes[2] << 16
        | input_bytes[3] << 24
    )
    return input


def xfer16(port, output):
    """Exchange 16 bit payloads with the GBA attached to the given serial port."""
    # Upper bits sent by master are zero
    input = xfer32(port, output & 0xFFFF)

    # Lower bits sent by slave are the same as previously sent by master, ignored
    return input >> 16


def upload(port_name, rom_path, timeout=10.0):
    """Upload a GBA multiboot ROM via the given serial port."""
    # Read ROM file
    with open(rom_path, "rb") as file:
        rom = file.read()

    # Multiboot ROM must fit the 256Ko RAM
    if len(rom) > 0x40000:
        raise ValueError("file too large")

    # Make sure size is a multiple of 16 bytes
    while len(rom) % 16 != 0:
        rom = rom + b"\0"

    # Open serial connection
    with serial.Serial(port_name) as port:
        # Send connection request, wait for slave to enter Normal mode
        with console.status("[cyan]Attempting to connect to GBA..."):
            start_time = time.time()

            while True:
                r = xfer16(port, 0x6202)

                # Check whether slave has entered correct mode, recognition okay
                if (r & 0xFFF0) == 0x7200:
                    # Slave replies with its ID (bit 1, 2, or 3, is set)
                    # Note: in Normal mode, there can be only one slave
                    x = r & 0xF
                    assert x == 2
                    break

                # Check for timeout
                if time.time() - start_time > timeout:
                    console.print("[red]Error:[/red] Connection timeout.")
                    sys.exit(1)

                # Wait a bit before trying again
                time.sleep(1 / 16)

        console.print("[green]✓[/green] Connected to GBA")

        # Exchange master/slave info
        r = xfer16(port, 0x6102)
        assert r == 0x7202

        # Send header
        for i in range(0, 0xC0, 2):
            # Send 2 bytes at a time
            xxxx = rom[i] | rom[i + 1] << 8
            r = xfer16(port, xxxx)

            # Slave replies with index and id
            assert ((r >> 8) & 0xFF) == (0xC0 - i) // 2
            assert (r & 0xFF) == 2

        # Transfer is complete
        r = xfer16(port, 0x6200)
        assert r == 2

        # Exchange master/slave info (again)
        r = xfer16(port, 0x6202)
        assert r == 0x7202

        # Choose palette
        # Note: we really don't care about this, but it seems we could configure it?
        pp = 0xD1

        # Wait until slave is ready
        while True:
            r = xfer16(port, 0x6300 | pp)
            if (r & 0xFF00) == 0x7300:
                break

        # Random client data
        cc = r & 0xFF

        # Compute handshake data
        # Note: client data for missing clients 2 and 3 are 0xff
        hh = (0x11 + cc + 0xFF + 0xFF) & 0xFF

        # Send handshake
        # Note: client returns a random value, which is ignored
        r = xfer16(port, 0x6400 | hh)
        assert (r & 0xFF00) == 0x7300

        # Wait a bit
        time.sleep(1 / 16)

        # Send length information
        llll = (len(rom) - 0xC0) // 4 - 0x34
        r = xfer16(port, llll)
        assert (r & 0xFF00) == 0x7300
        rr = r & 0xFF

        # Send encrypted payload
        c = 0xC387
        x = 0xC37B
        k = 0x43202F2F
        m = 0xFFFF0000 | cc << 8 | pp
        f = 0xFFFF0000 | rr << 8 | hh

        with Progress(
            SpinnerColumn(),
            TextColumn("[cyan]Uploading..."),
            BarColumn(),
            TaskProgressColumn(),
        ) as progress:
            task = progress.add_task("upload", total=(len(rom) - 0xC0) // 4)

            for i in range(0xC0, len(rom), 4):
                # Send 4 bytes at a time
                data = rom[i] | rom[i + 1] << 8 | rom[i + 2] << 16 | rom[i + 3] << 24

                # Update checksum
                c ^= data
                for _ in range(32):
                    carry = c & 1
                    c >>= 1
                    if carry:
                        c ^= x

                # Encrypt and send data
                m = (0x6F646573 * m + 1) & 0xFFFFFFFF
                complement = (-0x2000000 - i) & 0xFFFFFFFF
                yyyyyyyy = data ^ complement ^ m ^ k
                r = xfer32(port, yyyyyyyy)

                # Client replies with lower bits of destination address
                assert r >> 16 == i & 0xFFFF

                progress.update(task, advance=1)

        # Final checksum update
        c ^= f
        for _ in range(32):
            carry = c & 1
            c >>= 1
            if carry:
                c ^= x

        # Client just acknowledged the last bits
        r = xfer16(port, 0x0065)
        assert r == len(rom) & 0xFFFF

        # Wait until all slaves are ready for CRC transfer
        while True:
            r = xfer16(port, 0x0065)
            if r == 0x0075:
                break
            assert r == 0x0074

        # Signal that CRC will follow
        r = xfer16(port, 0x0066)
        assert r == 0x0075

        # Exchange CRC
        r = xfer16(port, c)
        assert r == c

        console.print("[green]✓[/green] Upload complete!")


def detect_pico_ports():
    """Detect all connected Raspberry Pi Pico devices."""
    return [
        port
        for port in serial.tools.list_ports.comports()
        if port.vid == PICO_VID and port.pid == PICO_PID
    ]


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="GBA multiboot ROM uploader", prog="multiboot"
    )
    parser.add_argument("path", help="Path to the GBA multiboot ROM file to upload")
    parser.add_argument(
        "-p",
        "--port",
        help="Serial device to use (auto-detected if omitted)",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=int,
        default=10,
        help="Connection timeout in seconds (default: 10)",
    )
    args = parser.parse_args()

    # Determine serial port
    if args.port:
        port_name = args.port
    else:
        # Find any connected Pico devices
        pico_ports = detect_pico_ports()

        # No devices found
        if not pico_ports:
            console.print(
                "[red]Error:[/red] No device found, use --port to specify port manually."
            )
            sys.exit(1)

        # Multiple devices found
        if len(pico_ports) > 1:
            console.print("[red]Error:[/red] Multiple serial devices found:")
            for port in pico_ports:
                console.print(f"  [cyan]{port.device}[/cyan]")

            console.print(
                f"\nSpecify one with: [cyan]--port {pico_ports[0].device}[/cyan]"
            )
            sys.exit(1)

        # Exactly one device found, use it
        port_name = pico_ports[0].device
        console.print(
            f"[green]✓[/green] Auto-detected device at [cyan]{port_name}[/cyan]"
        )

    # Perform upload
    upload(port_name, args.path, args.timeout)


if __name__ == "__main__":
    main()
