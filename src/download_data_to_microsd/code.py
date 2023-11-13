# SPDX-FileCopyrightText: 2021 Kattni Rembor for Adafruit Industries
# SPDX-License-Identifier: MIT
"""CircuitPython blink example for built-in NeoPixel LED"""
import time
import board
import digitalio

led = digitalio.DigitalInOut(board.D1)
led.direction = digitalio.Direction.OUTPUT

# Write your code here :-)
import board
import busio
import sdcardio
import storage

# Use the board's primary SPI bus
spi = board.SPI()

sd = sdioio.SDCard(
    clock=board.SDIO_CLOCK,
    command=board.SDIO_COMMAND,
    data=board.SDIO_DATA,
# Or, use an SPI bus on specific pins:
#spi = busio.SPI(board.SD_SCK, MOSI=board.SD_MOSI, MISO=board.SD_MISO)

# For breakout boards, you can choose any GPIO pin that's convenient:
cs = board.RX
try:
    sdcard = sdcardio.SDCard(spi, cs)
    vfs = storage.VfsFat(sdcard)

    storage.mount(vfs, "/sd")

    with open("/sd/test.txt", "w") as f:
        f.write("Hello world!\r\n")

    with open("/sd/test.txt", "r") as f:
        print("Read line from file:")
        print(f.readline())
except OSError as exc:
    print(exc)


while True:
    led.value = True
    time.sleep(0.5)
    led.value = False
    time.sleep(0.5)
