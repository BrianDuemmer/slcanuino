#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include "tCAN.h"
#include <RH_RF95.h>


#define LORA_PIN_NSS 5
#define LORA_PIN_INT 2
#define LORA_PIN_RESET 9

// Need to calibrate these to optimal data rate / transmission range.
// Calculated based off the RF95 datasheet in combination with the
// calculator at http://www.rfwireless-world.com/calculators/LoRa-Data-Rate-Calculator.html
#define LORA_FREQUENCY 912.5
#define LORA_SPREAD_FACTOR 8
#define LORA_BANDWIDTH 500000
#define LORA_CODE_RATE 1
#define LORA_TX_POWER 23
#define LORA_PREAMBLE 12

#define INFO_ID_BASE 990 			// Base ID for all info packets used for diagnostic about the connection

#define SEND_INFO_FRAME 1			// If true, sends a frame with connection metrics periodically
#define INFO_SEND_MILLIS 1000 		// The time between connection packets getting sent
#define INFO_FRAME_SENDER_ID (INFO_ID_BASE + 1)
#define INFO_FRAME_RECIEVER_ID (INFO_ID_BASE + 2)

#define SEND_TEST_FRAME 0 			// If true, will send a dummy test frame of incrementing numbers
#define TEST_FRAME_ID (INFO_ID_BASE + 0)

// If true, this permits messages recieved over LoRa to be forwarded to the network.
// Unless necessary, this will be set to false, as no encryption is used on the wireless traffic.
// If this is allowed, external information could be transmitted onto the bus, which would be a 
// big security risk
#define ALLOW_XMIT_FROM_LORA 0

/**Parameters for CAN serialization*/
#define CAN_DATA_FRAME 8 			// bytes / frame
#define CAN_ID_BYTES 4 				// bytes for destination ID
#define LORA_MSG_TERMINATOR '\n' 	// terminator for base64 encoded CAN data
#define LORA_MSG_INITIALIZER '~'	// initializer for base64 encoded CAN data
#define CAN_PACKET_SIZE 18			// Size of a serialized CAN frame

#ifdef __cplusplus
extern "C" {
#endif

// Populates buf with a base64 encoded / serialized representation of tCAN
void serialize(tCAN *frame, char *buf);

// Attempts to decode the serialized frame in buf to a tCan object.
// Returns 1 if the data in buf was decoded successfully, 0 on an error
char deserialize(tCAN *frame, char *buf);

// Starts up LoRa and configures it for operation in the system. If successful, flags that the lora is initialized
char initLora(RH_RF95 *rf95);

// sends a frame over LoRa
void sendFrame(tCAN *tc, RH_RF95 *rf95);

// Recieves a frame transmitted over LoRa
uint8_t recvFrame(tCAN *tc, RH_RF95 *rf95);

// resets the lora and asserts that the lora is uninitialized
void resetLora(RH_RF95 *rf95);

// Formats tc to a 1 byte frame of the given ID containing the reciever's last SNR. Returns 0 if the LoRa isn't initialized, 1 otherwise
uint8_t getInfoFrame(tCAN *tc, RH_RF95 *rf95, uint16_t id);

#ifdef __cplusplus
}
#endif

#endif // LORA_H