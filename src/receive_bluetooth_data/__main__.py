"""
SDP Team 1
Author: Patrick Walsh
Python main program for receiving bluetooth data from ESP32.
"""
import sys
import os
import time
from pathlib import Path
import asyncio
import logging
from bleak import BleakClient, BleakScanner
from typing import Final

# add assets directory to the path
sys.path.append(str(Path(os.path.dirname(__file__)).parent.parent / "assets"))

from CustomFormatter import CustomFormatter
import shank_thigh_send_pb2 as pb  # protocol buffer python formatted library

# set up logging

LOG_FORMAT = ["[%(asctime)s] [%(levelname)s]", "%(message)s "]

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)
LOG_FILE = "receive_bluetooth_data.log"
logger = logging.FileHandler(LOG_FILE)
logger.setFormatter("".join(LOG_FORMAT))
log.addHandler(logger)
logger.setLevel(logging.DEBUG)

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(CustomFormatter())
log.addHandler(ch)

# global variables
NAME = "ESP32"
CHANNEL_NAME = "C2431FC1-6A48-D48C-FEBA-7AA454D2B165"
CHARACTERISTIC_ID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

FREQUENCY: Final[float] = 1 / 15  # 15 Hz


async def main(address):
    # discover all possible connections
    devices = await BleakScanner.discover()
    print("\n".join([str(d) for d in devices]))
    async with BleakClient(address) as client:
        # send hello world through bluetooth
        # read the response
        while True:
            response = await client.read_gatt_char(CHARACTERISTIC_ID)
            print(response)
            print(response.decode())
            print(len(response.decode()))

            time.sleep()


asyncio.run(main(CHANNEL_NAME))
