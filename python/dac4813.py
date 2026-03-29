"""
DAC4813 Python client library.

Requires: pyserial

Usage:
    from dac4813 import DAC4813

    with DAC4813("/dev/ttyACM0") as dac:
        print(dac.identify())   # "DAC4813 on Arduino Uno Rev. 3"
        dac.set(1, 0x800)       # channel 1 → 0 V
        dac.set(2, 0x000)       # channel 2 → -10 V
        dac.reset()             # all channels → 0 V (0x800)
"""

import time
import serial


class DAC4813Error(Exception):
    """Raised when the firmware returns ERR or an unexpected response."""


class DAC4813:
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 1.0,
                 reset_delay: float = 2.0):
        """Open *port* and wait for the Arduino bootloader to finish.

        Opening the serial port toggles DTR, which resets the Arduino.
        *reset_delay* (default 2 s) is the time to wait before sending any
        commands.  Pass ``reset_delay=0`` if the Arduino is already running
        (e.g. the port was kept open) or if DTR reset is disabled on the board.
        """
        self._ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        if reset_delay > 0:
            time.sleep(reset_delay)

    # ── low-level I/O ────────────────────────────────────────────────────

    def _send(self, command: str) -> str:
        self._ser.write((command + "\n").encode())
        response = self._ser.readline().decode().strip()
        if response == "ERR":
            raise DAC4813Error(f"Firmware rejected command: {command!r}")
        return response

    # ── public API ───────────────────────────────────────────────────────

    def set(self, channel: int, value: int) -> None:
        """Load *value* (0x000–0xFFF) into DAC *channel* (1–4).

        Voltage mapping: 0x000 = -10 V, 0x800 = 0 V, 0xFFF ≈ +10 V.
        """
        if channel not in range(1, 5):
            raise ValueError(f"channel must be 1–4, got {channel}")
        if not (0 <= value <= 0xFFF):
            raise ValueError(f"value must be 0x000–0xFFF, got {value:#05x}")
        self._send(f"{channel} {value:03x}")

    def reset(self) -> None:
        """Reset all four DAC channels to 0x800 (0 V)."""
        self._send("*RST")

    def identify(self) -> str:
        """Return the firmware identification string."""
        return self._send("*IDN?")

    # ── context manager ──────────────────────────────────────────────────

    def close(self) -> None:
        self._ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
