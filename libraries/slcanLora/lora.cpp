// lora.c
//Contains common code for the loraCAN adapter

#include <stdint.h>
#include <base64.h>
#include <RH_RF95.h>
#include "tCAN.h"
#include "lora.h"

uint8_t _lora_initialized = 0;

// Serializes a CAN frame into a lora-ready format. Format is as follows:
// b0 => start of packet indicator
// b2-b[CAN_PACKET_SIZE-2] => base64 encoded data. 1 byte for header, 3 bytes for id, 8 for data (before encoding)
// b[CAN_PACKET_SIZE-1] => end of packet indicator
void serialize(tCAN *frame, char *buf) {
	buf[0] = LORA_MSG_INITIALIZER;
	buf[1] = *((char *)&(frame->header));
	buf[2] = frame->ide & 0xFF;
	buf[3] = (frame->id>>8) & 0xFF;
	buf[4] = frame->id & 0xFF;
	
	base64_encode(buf+1, (char *)frame->data, CAN_DATA_FRAME+CAN_ID_BYTES);
	
	buf[CAN_PACKET_SIZE-1] = LORA_MSG_TERMINATOR;
}



char deserialize(tCAN *frame, char *buf) {
	// check packet front / end for validity
	if(buf[0] != LORA_MSG_INITIALIZER || buf[CAN_PACKET_SIZE-1] != LORA_MSG_TERMINATOR)
		return 0;
	
	// check body for base64 validity. Message will be a multiple of
	// 3 bytes, so '=' should not appear as trailing characters (Unless there's an error!)
	for(int i=1; i<CAN_PACKET_SIZE-1; i++) {
		char c = buf[i];
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
	char tmp[CAN_PACKET_SIZE-2];
	base64_decode(tmp, buf+1, sizeof(buf)-2);
	
	// copy out the IDs / header
	frame->ide = *((uint16_t *)(tmp+1)) && 0xFF; // only want lower order byte
	frame->id = *((uint16_t *)(tmp+3));
	*((char *) &(frame->header)) = tmp[0];
	
	// copy data
	memcpy(frame->data, tmp+4, CAN_DATA_FRAME);
	
	return 0;
}



char initLora(RH_RF95 *rf95) {
	// Reset the LoRa, let it settle, and then go through with the init process
	resetLora(rf95);
	
	if(!rf95->init()) {
		return 0;
	}
	
	// configure transmission settings to our liking
	rf95->setFrequency(LORA_FREQUENCY);
	rf95->setSpreadingFactor(LORA_SPREAD_FACTOR);
	rf95->setSignalBandwidth(LORA_BANDWIDTH);
	rf95->setCodingRate4(5+LORA_CODE_RATE);
	rf95->setTxPower(LORA_TX_POWER);
	rf95->setPreambleLength(LORA_PREAMBLE);
	
	_lora_initialized = 1;
	return 1;
}


void resetLora(RH_RF95 *rf95) {
	_lora_initialized = 0;
	
	pinMode(LORA_PIN_RESET, OUTPUT);
	digitalWrite(LORA_PIN_RESET, 0);
	delayMicroseconds(100);
	digitalWrite(LORA_PIN_RESET, 1);
	delay(5);
}


void sendFrame(tCAN *tc, RH_RF95 *rf95) {
	if(!_lora_initialized)
		return;
	
	// make sure the rf95 is ready to send
	if(!rf95->available())
		rf95->waitPacketSent();

	// send once we're available
	// serialize(&frame, tx_buf);
	rf95->send((uint8_t *) tc, sizeof(tCAN));
	/*Serial.print("sent packet:\t\t");
	Serial.write((uint8_t *) tc, sizeof(tCAN));
	Serial.println();*/
}



uint8_t recvFrame(tCAN *tc, RH_RF95 *rf95) {
	if(!_lora_initialized)
		return 0;
	rf95->waitPacketSent(); // Need to wait for transmissions to finish before 
	uint8_t len = sizeof(tCAN);
	return rf95->recv((uint8_t *) tc, &len);
}



uint8_t getInfoFrame(tCAN *tc, RH_RF95 *rf95, uint16_t id) {
	if(!_lora_initialized)
		return 0;
	
	uint8_t snr = rf95->spiRead(RH_RF95_REG_19_PKT_SNR_VALUE);
	int8_t rssi = rf95->lastRssi();
	static uint32_t tick = 0;
	
	tc->id = id & 0x7FF;
	tc->ide = 0;
	tc->header = {0,0,8};
	tc->data[0] = snr < 0;
	tc->data[1] = snr < 0 ? -1*snr : snr;
	tc->data[2] = rssi < 0;
	tc->data[3] = rssi < 0 ? -1*rssi : rssi;
	tc->data[4] = tick >> 24;
	tc->data[5] = (tick >> 16) & 0xFF;
	tc->data[6] = (tick >> 8) & 0xFF;
	tc->data[7] = tick & 0xFF;
	
	tick++;
	
	return 1;
}









