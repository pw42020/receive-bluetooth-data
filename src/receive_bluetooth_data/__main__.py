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
NAME = "ESP32"
CHANNEL_NAME = "C2431FC1-6A48-D48C-FEBA-7AA454D2B165"
LEFT_LEG_ID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
RIGHT_LEG_ID = "797e275d-3583-4403-8815-0e56a47ed5fa"

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
        if device.name == "STRIDESYNCLEGPROCESSOR":
            chosen_device = device
        log.debug("Name: %s, address: %s", device.name, device.address)

    if chosen_device is None:
        log.critical("Could not find device")
        return

    async with BleakClient(chosen_device.address) as client:
        # read data from bluetooth
        # read all of client's characteristics
        log.info("Connected: %s", await client.is_connected())
        # print all services
        for service in client.services:
            log.info("Service: %s", service)
        try:
            file = create_run_file()
            # writing header for csv file
            file.write(
                "".join(
                    [
                        "l_shank_x,l_shank_y,l_shank_z,l_thigh_x,l_thigh_y,l_thigh_z,"
                        "r_shank_x,r_shank_y,r_shank_z,r_thigh_x,r_thigh_y,r_thigh_z\n"
                    ]
                )
            )
            while True:
                left_leg_response: Final[str] = await client.read_gatt_char(LEFT_LEG_ID)
                right_leg_response: Final[str] = await client.read_gatt_char(
                    RIGHT_LEG_ID
                )

                # add data to file
                file.write(
                    left_leg_response.decode()
                    + ","
                    + right_leg_response.decode()
                    + "\n"
                )

                # give time for program to send more packets
                time.sleep(round(1 / SAMPLES_PER_SECOND))
        finally:
            # closing the file when the program is complete or an error occurs
            file.close()


if __name__ == "__main__":
    # asyncio.run(main(CHANNEL_NAME))
    file = create_run_file()
    file.write(
        "".join(
            [
                "l_shank_x,l_shank_y,l_shank_z,l_thigh_x,l_thigh_y,l_thigh_z,"
                "r_shank_x,r_shank_y,r_shank_z,r_thigh_x,r_thigh_y,r_thigh_z\n"
            ]
        )
    )
