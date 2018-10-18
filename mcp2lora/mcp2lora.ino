#include <RH_RF95.h>
#include <Base64.h>
#include <mcp_can.h>
#include <lora.h>

#define CAN_SPEED CAN_1000KBPS

#define MCP_PIN_INT 3
#define MCP_PIN_NSS 10

RH_RF95 rf95(LORA_PIN_NSS, LORA_PIN_INT);
MCP_CAN mcp(MCP_PIN_NSS);

char tx_buf[CAN_PACKET_SIZE];

uint8_t readMcp(tCAN *);
uint8_t writeMcp(tCAN *);
void sendTestFrame();


void setup() {
  Serial.begin(115200);

  // need to assert the output of this pin so the arduino will work in SPI master mode
  pinMode(SPI_PIN_ASSERT, OUTPUT);
  digitalWrite(SPI_PIN_ASSERT, 1);

  // Make sure an inconsistent state won't reset the lora
  pinMode(LORA_PIN_RESET, OUTPUT);
  digitalWrite(LORA_PIN_RESET, 1);
  
  while (CAN_OK != mcp.begin(CAN_SPEED))              // init can bus : baudrate = 500k
  {
      Serial.println("CAN BUS Shield init fail");
      Serial.println(" Init CAN BUS Shield again");
      delay(1000);
  }
  Serial.println("CAN BUS Shield init ok!");
  
  if(initLora(&rf95)) {
    Serial.println("LoRA Init OK!");
  } else {
    Serial.println("LoRA init Fail!");
  }
}


void loop() {
  static uint32_t lastInfo = 0;
  tCAN tc;
  tCAN tcInfo;
    
  if(recvFrame(&tc, &rf95) && ALLOW_XMIT_FROM_LORA) // We got a frame in
    writeMcp(&tc);

  if(millis() >= lastInfo + INFO_SEND_MILLIS) {
    if(getInfoFrame(&tcInfo, &rf95, INFO_FRAME_SENDER_ID)) {
  
      // Send the info frame both here and the socketcan client
      sendFrame(&tcInfo, &rf95);
      writeMcp(&tcInfo);
      lastInfo = millis();
    }
  }

  if(readMcp(&tc))
    sendFrame(&tc, &rf95);
    
  if(SEND_TEST_FRAME)
    sendTestFrame();
}


uint8_t readMcp(tCAN *tc) {
  if(CAN_MSGAVAIL == mcp.checkReceive()) {

    // get a tCAN object out of the current frame
    unsigned long id;
    byte len;
    
    mcp.readMsgBufID(&id, &len, tc->data);
    tc->header.length = len & 0xF;
    tc->id = id & 0x7FF;
    tc->header.rtr = mcp.isRemoteRequest();
    
    if(mcp.isExtendedFrame()) {
      tc->header.ide = 1;
      tc->ide = id >> 11;
    } else {
      tc->header.ide = 0;
      tc->ide = 0;
    }

    return 1;
  } else
    return 0;
}



uint8_t writeMcp(tCAN *tc) {
  uint32_t id = tc->id;
  if(tc->header.ide)
    id |= ((uint32_t)tc->ide) << 16;
    
  return mcp.sendMsgBuf(
      id,                 // msg ID
      tc->header.ide,     // extended ID
      tc->header.rtr,     // remote reqest
      tc->header.length,  // data length
      tc->data,           // data buffer
      1                   // wait sent
  );
}



void sendTestFrame() {
  static uint32_t count = 0;

  tCAN test1 = {TEST_FRAME_ID, 0, {0,0, 8}, {0}};
  test1.data[0] = 'F';
  test1.data[1] = 'o';
  test1.data[2] = 'o';
  test1.data[7] = 0;
  memcpy(test1.data+3, &count, 4);
  count++;

  sendFrame(&test1, &rf95);
}















