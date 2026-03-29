MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = main
PORT   ?= /dev/ttyACM0
BAUD    = 115200

CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -std=c99

.PHONY: all build flash clean

all: build

build: $(TARGET).hex

$(TARGET).elf: $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: build
	$(AVRDUDE) -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$(TARGET).hex

clean:
	rm -f $(TARGET).elf $(TARGET).hex
