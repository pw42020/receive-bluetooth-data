"""
SDP Team 1
Author: Patrick Walsh
Python main program for receiving bluetooth data from ESP32.

Notes
-----

1. Important Note: This module uses Python 3.8 as that is the only version that works
with Bleak. If you are looking to run this program, please make sure you have
Python 3.8 installed, which you can look at (pyenv)[https://amaral.northwestern.edu/resources/guides/pyenv-tutorial]
for what I believe is the easiest/best way to install and create a virtual environment
for Python 3.8.

2. The program is designed to receive data from the ESP32 via bluetooth and write
the data to a file. The data is received in the form of bytes and is written to
the file in the form of a string of comma separated values.

3. The format of the incoming bluetooth data is a string of comma separated
values as:
`<l_shank_x>,<l_shank_y>,<l_shank_z>,<l_thigh_x>,<l_thigh_y>,<l_thigh_z>`

4. and data is saved in the .run file as:
`l_shank_x,l_shank_y,l_shank_z,l_thigh_x,l_thigh_y,l_thigh_z,r_shank_x,r_shank_y,r_shank_z,r_thigh_x,r_thigh_y,r_thigh_z`
"""

from .__main__ import main, create_run_file, NAME, LEG_ID

__version__ = "0.1.0"
