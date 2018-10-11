// loraCanReciever.cpp

// CAN driver class for the slcanduino library for emulating
// a socketcan device with arduino. Instead of using an mcp2515
// CAN chip as this library is designed for, we instead
// encode CAN traffic over a LoRA wireless link, from a 
// remote location (the solar car). While transmitting
// data to the car is not implemented, Data recieved
// over LoRA will be checked for validity, and decoded
// into CAN objects, buffered, and the main library's
// support code will manage all the socketcan communications.

#include <stdint.h>
#include <RH_RF95.h>
#include "Canbus.h"

