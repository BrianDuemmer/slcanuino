#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include "tCan.h"
#include <RH_RF95>

extern RH_RF95 rf95;

#define LORA_PIN_NSS 10
#define LORA_PIN_INT 3

// Need to calibrate these to optimal data rate / transmission range.
// Calculated based off the RF95 datasheet in combination with the
// calculator at http://www.rfwireless-world.com/calculators/LoRa-Data-Rate-Calculator.html
#define LORA_SPREAD_FACTOR 7
#define LORA_BANDWIDTH 500000
#define LORA_CODE_RATE 1
#define LORA_TX_POWER 20
#define LORA_PREAMBLE 12

/**Parameters for CAN serialization*/
// bytes / frame
#define CAN_DATA_FRAME 8

// bytes for destination ID
#define CAN_ID_BYTES 4

// terminator for base64 encoded CAN data
#define LORA_MSG_TERMINATOR '\n'

// initializer for base64 encoded CAN data
#define LORA_MSG_INITIALIZER '~'

// Size of a serialized CAN frame
#define CAN_PACKET_SIZE 6



#ifdef __cplusplus
extern "C"
{
#endif

// Populates buf with a base64 encoded / serialized representation of tCan
void serialize(tCan *frame, uint8_t *buf);

// Attempts to decode the serialized frame in buf to a tCan object.
// Returns 1 if the data in buf was decoded successfully, 0 on an error
uint8_t deserialize(tCan *frame, uint8_t *buf)

// Starts up LoRa and configures it for operation in the system
uint8_t initLora();

#ifdef __cplusplus
}
#endif

#endif // LORA_H