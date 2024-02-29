"""
SDP Team 1
Author: Patrick Walsh
Python main program for receiving bluetooth data from ESP32.

Notes
-----
The format of the incoming bluetooth data is a string of comma separated
values as:
    <l_shank_x>,<l_shank_y>,<l_shank_z>,<l_thigh_x>,<l_thigh_y>,<l_thigh_z>

and data is saved in the .run file as:
    l_shank_x,l_shank_y,l_shank_z,l_thigh_x,l_thigh_y,l_thigh_z,r_shank_x,r_shank_y,r_shank_z,r_thigh_x,r_thigh_y,r_thigh_z

"""

import sys
import os
import time
import math
import struct
from pathlib import Path
import asyncio
import logging
from datetime import datetime
from typing import Final, TextIO

from bleak import BleakClient, BleakScanner

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
NAME = "StrideSync"
# CHANNEL_NAME = "C2431FC1-6A48-D48C-FEBA-7AA454D2B165"
LEG_ID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

CHANNEL_NAME = "4BE89814-827A-7A33-0672-DE98DFF5888D"

NUM_FLOATS_TRANSMITTED: Final[int] = 6

SAMPLES_PER_SECOND: Final[int] = 15

RUN_FOLDER_LOCATION: Final[Path] = Path(os.path.dirname(__file__)).parent.parent


def create_run_file() -> TextIO:
    """create current session with user of their run into currently
    hardcoded .run file that will act as a CSV

    Notes
    -----
    The file in specific will stay encoded inside of the .run in utf-8"""
    run_folder: Final[str] = str(RUN_FOLDER_LOCATION / "runs")
    if not os.path.exists(run_folder):
        os.mkdir(run_folder)

    # create new run session
    folder_name: Final[str] = datetime.now().strftime("%Y-%m-%dT%H-%M-%SZ")
    log.info("Folder is named %s", folder_name)
    os.mkdir(f"{run_folder}/{folder_name}")
    # create file
    return open(f"{run_folder}/{folder_name}/data.run", "w")


async def main(address):
    """main function for receiving bluetooth data from ESP32

    Parameters
    ----------
    address: str
        address of bluetooth device
    """
    # discover all devices
    devices = await BleakScanner.discover()
    # print all devices
    chosen_device = None
    for device in devices:
        if device.name == NAME:
            chosen_device = device
        log.debug("Name: %s, address: %s", device.name, device.address)

    if chosen_device is None:
        log.critical("Could not find device")
        return

    start = time.time()
    async with BleakClient(chosen_device.address) as client:
        # read data from bluetooth
        # read all of client's characteristics
        log.info("Connected: %s", await client.is_connected())
        file_lines: list[str] = []
        # print all services
        for service in client.services:
            log.info("Service: %s", service)
        try:
            file = create_run_file()
            # writing header for csv file
            file_lines.append(
                "".join(
                    [
                        "l_thigh_x,l_thigh_y,l_thigh_z,l_shank_x,l_shank_y,l_shank_z,",
                        "r_thigh_x,r_thigh_y,r_thigh_z,r_shank_x,r_shank_y,r_shank_z\n",
                    ]
                )
            )
            while True:
                leg_response: Final[bytearray] = await client.read_gatt_char(LEG_ID)
                try:
                    if leg_response.decode() == "GOOD":
                        continue
                except Exception as e:
                    log.error("Error: %s", e)

                print(leg_response)
                try:
                    if leg_response.decode() != "DONE" or leg_response.decode() == "":
                        await client.write_gatt_char(LEG_ID, b"GOOD")
                        continue
                except Exception as e:
                    log.error("Error: %s", e)

                # add data to file
                try:
                    if leg_response.decode() != "DONE":
                        float_list = struct.unpack("%sf" % 12, bytearray(leg_response))
                        file_lines.append(",".join(str(e) for e in float_list) + "\n")
                        await client.write_gatt_char(LEG_ID, b"GOOD")
                    else:
                        log.info("DONE")
                        break
                    # time.sleep(0.005)
                except Exception as e:
                    float_list = struct.unpack("%sf" % 12, bytearray(leg_response))
                    print(float_list)
                    file_lines.append(",".join(str(e) for e in float_list) + "\n")
                    await client.write_gatt_char(LEG_ID, b"GOOD")

                # give time for program to send more packets
                # time.sleep(round(1 / SAMPLES_PER_SECOND))
        finally:
            # closing the file when the program is complete or an error occurs
            file.writelines(file_lines)
            file.close()

    end = time.time()
    log.info("Time: %s", end - start)


if __name__ == "__main__":
    asyncio.run(main(CHANNEL_NAME))
    # file.write(
    #     "".join(
    #         [
    #             "l_shank_x,l_shank_y,l_shank_z,l_thigh_x,l_thigh_y,l_thigh_z,"
    #             "r_shank_x,r_shank_y,r_shank_z,r_thigh_x,r_thigh_y,r_thigh_z\n"
    #         ]
    #     )
    # )
