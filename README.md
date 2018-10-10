# Slcanuino

Original repo: https://github.com/kahiroka/slcanuino

This is an Arduino sketch which makes an Arduino recieving serialized CAN traffic over LoRa wireless into a CAN-USB adapter for Linux SocketCAN(can-utils). Library files under Canbus folder originaly comes from 'CAN-BUS ECU Reader demo sketch v4' on skpang and were modified. The files should be copied under ~/Arduino/libraries/ to compile the sketch file:'slcan.ino'.



# Supported hardware

Tested using the following LoRa shield and CAN bus adapter (for transmission):
http://www.dragino.com/products/module/item/102-lora-shield.html
http://wiki.seeedstudio.com/CAN-BUS_Shield_V1.2/


# How to use

Burn your Arduino with this and install can-utils for your linux environment in advance.

## Deps
1. slcan (kernel module)
2. SocketCAN (http://www.pengutronix.de/software/libsocketcan/)
3. can-utils (https://github.com/linux-can/can-utils)


## Setup

    $ sudo slcan_attach -f -s5 -o /dev/ttyUSB0  
    $ sudo slcand -S 500000 ttyUSB0 can0  
    $ sudo ifconfig can0 up  

then,

    $ candump can0

## Cleanup

    $ sudo ifconfig can0 down  
    $ sudo killall slcand  
