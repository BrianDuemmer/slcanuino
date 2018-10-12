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
#include "lora.h"
#include "tCan.h"
#include "Canbus.h"

// lora.c
//Contains common code for the loraCAN adapter

#include <stdint.h>
#include <base64.h>
#include "tCan.h"
#include "lora.h"


// Serializes a CAN frame into a lora-ready format. Format is as follows:
// b0 => start of packet indicator
// b2-b[CAN_PACKET_SIZE-2] => base64 encoded data. 1 byte for header, 3 bytes for id, 8 for data (before encoding)
// b[CAN_PACKET_SIZE-1] => end of packet indicator
void LoraCan::serialize(tCan *frame, uint8_t *buf) {
	buf[0] = LORA_MSG_INITIALIZER;
	buf[1] = frame->header;
	buf[2] = frame->ide & 0xFF;
	buf[3] = (frame->id>>8) & 0xFF;
	buf[4] = frame->id & 0xFF;
	
	base64_encode(buf+1, frame->data, CAN_DATA_FRAME+CAN_ID_BYTES);
	
	buf[CAN_PACKET_SIZE-1] = LORA_MSG_TERMINATOR;
}



uint8_t LoraCan::deserialize(tCan *frame, uint8_t *buf) {
	// check packet front / end for validity
	if(buf[0] != LORA_MSG_INITIALIZER || buf[CAN_PACKET_SIZE-1] != LORA_MSG_TERMINATOR)
		return 0;
	
	// check body for base64 validity. Message will be a multiple of
	// 3 bytes, so '=' should not appear as trailing characters (Unless there's an error!)
	for(int i=1; i<CAN_PACKET_SIZE-1; i++) {
		uint8_t c = buf[i];
		if(
			(c>='A' && c<='Z')	||
			(c>='a' && c<='z')	||
			(c>='0' && c<='9')	||
			c=='+'				||
			c=='/'
		)
		return 0;
	}
	
	// If we make it here, the data is *probably* valid. We may add smarter
	// integrity checks later, but until then we'll just decode this as is.
	uint8_t tmp[CAN_PACKET_SIZE-2];
	base64_decode(tmp, buf+1, sizeof(tmp));
	
	// copy out the IDs / header
	frame->ide = *((uint16_t *)buf) && 0xFF; // only want lower order byte
	frame->id = *((uint16_t *)(buf+2));
	frame->header = buf[1];
	
	// copy data
	memcpy(frame->data, tmp+4, CAN_DATA_FRAME);
	
	return 0;
}

uint8_t LoraCan::init(uint8_t speed) {
	return init();
}

uint8_t LoraCan::init() {
	if(!rf95.init()) {
		return 0;
	}
	
	// configure transmission settings to our liking
	rf95.setSpreadingFactor(LORA_SPREAD_FACTOR);
	rf95.setSignalBandwidth(LORA_BANDWIDTH);
	rf95.setCodingRate4(5+LORA_CODE_RATE);
	rf95.setTxPower(LORA_TX_POWER);
	rf95.setPreambleLength(LORA_PREAMBLE);
	rf95.setFrequency(LORA_FREQ);
	
	return 1;
}



char LoraCan::message_tx(tCan *msg) {
	serialize(msg, tx_buf);
	return rf95.send(tx_buf);
}


char LoraCan::message_rx(tCan *msg) {
	uint8_t len;
	// messages longer than the packet size *may* be ok thanks to buffering issues, 
	// so allow them. If it's shorter though, that's no good, so ignore it
	uint8_t valid =	rf95.recv(rx_buf, &len) &&	// If a valid message has been read...
					len >= CAN_PACKET_SIZE && 	// And it has correct size...
					deserialize(msg, rx_buf);	// And it was decoded properly..
	
	// ...return true. If any of that fails, return false
	return valid;
}









