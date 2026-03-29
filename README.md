# arduino_dac4813
Arduino-controlled DAC4813 - 4ch +/-10V analog out

## Circuit Description

The basic board used here is the Arduino Uno Rev. 3.

The wiring of the DAC4813 to the Arduino and ATmega328 on there is as follows:

| AVR pin | Arduino pin | DAC4813 |
|---------|-------------|---------|
| PD2     | Digital 2   | D3      |
| PD3     | Digital 3   | D2      |
| PD4     | Digital 4   | D1      |
| PD5     | Digital 5   | D0      |
| PD6     | Digital 6   | D11     |
| PD7     | Digital 7   | D10     |
| PB0     | Digital 8   | D9      |
| PB1     | Digital 9   | D8      |
| PB2     | Digital 10  | D7      |
| PB3     | Digital 11  | D6      |
| PB4     | Digital 12  | D5      |
| PB3     | Digital 13  | D4      |
| PC0     | Analog 0    | /WR     |
| PC1     | Analog 1    | /EN1    |
| PC2     | Analog 2    | /EN2    |
| PC3     | Analog 3    | /EN3    |
| PC4     | Analog 4    | /EN4    |
| PC5     | Analog 5    | /LDAC   |

The 12 data bits D0 ... D11 of the DAC4813 are mapped to the output voltage range from -10V to +10V as follows:
```
0x000 -> -10V
0x800 -> 0V
0xFFF -> +10V - 1 LSB
```

The `/RESET` pin of the DAC4813 is connected to +5V via a 10k resistor, with a 1uF capacitor to GND to make for a switch-on reset of the DAC4813.
A 1N4148 diode is connected from /RESET to +5V to quickly discharge the 1uF capacitor into the +5V rail in case of a brief power outage, in order to ensure a correct reset of the DAC4813.

In order to load a new data word into a specific DAC channel (1 ... 4), the respective `/EN` signal needs to be programmed low.
All other `/EN` signals should be held high, in order to only load a specific DAC channel.
Then, the data bits are loaded into the specific DAC channel when both `/WR` and `/LDAC` are programmed low.
In the idle state, all `/EN` signals as well as `/WR` and `/LDAC` should be held high.

## Remote Control

The Arduino code is written in C for use with `avr-gcc`.

It accepts commands of the following format:
```
<ch> <value>\n
```
where `<ch>` is an integer between 1 and 4 to select the DAC channel to program and `<value>` is a 3-digit hex value between `000` and `fff`/`FFF` which is the value to load into the respective DAC channel.
The newline `\n` terminates the command.
The DAC is updated immediately and the command is echoed back upon successful parsing of the command.
In case the command could not be parsed successfully, `ERR\n` is echoed back.

All channels can be reset to `0x800` corresponding to an output voltage of 0V by sending the `*RST\n` command.

A brief identifier string can be requested by sending `*IDN?\n`:
```
DAC4813 on Arduino Uno Rev. 3
```

