# Telescope proxy
This repository contains the code for the LX200GPS ACF AutoStarII telescope command proxy. It's purpose is to interface an ESP32 DevKit V1 with the serial port of the telescope, and provide it as an HTTP resource. Attached is the official LX200 command set protocol released by the manifacturer. The entire structure of this firmware is prototyped in serial_config.h.
## Global Variables
Every time the micro queries the telescope for its position they update the global variables with the current coordinates of the telescope.
## LX200 functions
These functions act as a interface with the HardwareSerial library to manage the UART interface. These functions all depend on lx200_send_and_recieve_CMD to do their bidding and use the #defines to get the correct command string.
## Callback functions
These functions manage the requests incoming from the HTTP interface and call the respective LX200 function, parsing strings and checking responses.
## Resources Provided
This version of the firmware provides very basic functions to get the current coordinates of the telescope and set target coordinates and start the slewing. Information gets returned as a JSON via a TCP socket with the keys defined in JSON_key_names.
## info.md
This file is a dump of commands and them getting tested on a terminal connected directly from the machine to the RS232 port of the telescope.
