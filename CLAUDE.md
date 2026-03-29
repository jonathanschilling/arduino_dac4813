# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino Uno Rev. 3 firmware (C, avr-gcc) for driving a DAC4813 quad-channel ±10V DAC via a parallel data bus. The firmware exposes a serial command interface for remote control.

## Build & Flash

Firmware lives in `firmware/`. Use the Makefile:

```sh
cd firmware
make              # compile → main.elf + main.hex
make flash        # flash via avrdude (default port /dev/ttyACM0)
make flash PORT=/dev/ttyUSB0   # override port
make clean
```

The Makefile uses `avr-gcc` with U2X baud mode (UBRR=16) for 115200 baud at 16 MHz.

## Hardware Interface

**Data bus** — 12-bit parallel (D0–D11), split across PORTD (PD2–PD7 → D3–D0, D11, D10) and PORTB (PB0–PB5 → D9–D4):

| AVR pin | Arduino pin | DAC4813 |
|---------|-------------|---------|
| PD2–PD7 | Digital 2–7 | D3–D0, D11, D10 |
| PB0–PB5 | Digital 8–13 | D9–D4 |

**Control signals** — all active-low, on PORTC:

| AVR pin | Arduino pin | Signal |
|---------|-------------|--------|
| PC0 | Analog 0 | /WR  |
| PC1 | Analog 1 | /EN1 |
| PC2 | Analog 2 | /EN2 |
| PC3 | Analog 3 | /EN3 |
| PC4 | Analog 4 | /EN4 |
| PC5 | Analog 5 | /LDAC |

**DAC voltage mapping:** `0x000` = −10 V, `0x800` = 0 V, `0xFFF` = +10 V − 1 LSB.

**Write sequence:** assert `/ENx` low for the target channel, present data bits, assert `/WR` and `/LDAC` low simultaneously, then deassert all. All other `/EN` lines must stay high during a write.

## Serial Command Protocol

UART at the Arduino default baud rate. Commands are terminated by `\n`.

| Command | Description |
|---------|-------------|
| `<ch> <hex>\n` | Set channel `ch` (1–4) to 12-bit hex value `000`–`fff`. DAC updates immediately; command is echoed on success. |
| `*RST\n` | Reset all channels to `0x800` (0 V). |
| `*IDN?\n` | Returns `DAC4813 on Arduino Uno Rev. 3`. |

On parse failure the firmware replies `ERR\n`.
