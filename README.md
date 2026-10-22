# Telescope proxy
This repository contains the code for the LX200GPS ACF AutoStarII telescope command proxy. It's purpose is to interface an ESP32 DevKit V1 with the serial port of the telescope, and provide it as an HTTP resource. Attached is the official LX200 command set protocol released by the manifacturer. The entire structure of this firmware is prototyped in serial_config.h.
## Global Variables
Every time the micro queries the telescope for its position they update the global variables with the current coordinates of the telescope.
There is currentRA, currentDA that store the current DA and RA and is updated every time the position is queried to the telescope. The targetRA and DA instead are used by the set functions to "remember" what position the telescope should reach.
## LX200 functions
These functions act as a interface with the HardwareSerial library to manage the UART interface. These functions all depend on lx200_send_and_recieve_CMD to do their bidding and use the #defines to get the correct command string. The commands used are highlighted on the lx200 command set document. There is an infinity of commands to be implemented but its outside of the scope of this project. 
## Callback functions
These functions manage the requests incoming from the HTTP interface and call the respective LX200 function, parsing strings and checking responses.
## Resources Provided
This version of the firmware provides very basic functions to get the current coordinates of the telescope and set target coordinates and start the slewing. Information gets returned as a JSON via a TCP socket with the keys defined in JSON_key_names.
## info.md
This file is a dump of commands and them getting tested on a terminal connected directly from the machine to the RS232 port of the telescope. It can be useful since the document isnt really accurate on the responses the telescope gives out. 
# REST API TREE
/lx200/ 
    /network/
        /tcp-socket/    -> PUT:  sets tcp-server socket config (ip , port),
                                 ip is necessary, port is 7000 by default
                                 set this params via query (not 100% REST but saves ram. Might change it later.)
                        -> GET:  get current tcp socket
    /settings/
        /resetESP/  -> PUT: reset esp
        /resetLX/   -> PUT: send :I# to the telescope
        /heartbeat/ -> GET: "quick connectivity check"
    /position/      -> GET: get both coordinates
                    -> PUT: set both coords and go: position?RA=DD:MM:SS&DA=sDD:MM:SS
        /RA/        -> GET: get the RA coord string
                    -> PUT: set RA with a query and go: RA?RA=DD:MM:SS
        /DA/        -> GET: get DA coord string
                    -> PUT: set DA with a query and go: DA?DA=sDD:MM:SS, dont worry about the evil 0xDF char as it is handled
        /home/      -> PUT: not implemented
    /stop/
