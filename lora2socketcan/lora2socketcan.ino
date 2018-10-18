// Need this to appease the compiler gods. If The code fails to compile
// with more unholy errors in the future referrring to this function, 
// maybe comment it out?
void send_canmsg(char *buf) __attribute__((__optimize__("O2")));

#include <RH_RF95.h>
#include <Base64.h>
#include <lora.h>

// the CANdapter software will initially poll all COM ports
// at 9600 baud and check that it responds correctly to a version
// ("V\r") command, which should return something of the form
// "Vxxxxxx", where x can be any digit. It then will shift
// to a much higher baud for operation. While the real
// CANdapter probably has the capability to conform to baud shifts
// considering it worked from a terminal emulator at 115200, but
// Arduino doesn't, so we must set our baud manually. Hopefully
// they don't change this constant
#define INITIAL_BAUD 9600
#define RUNNING_BAUD 921600

#define LED_OPEN 13
#define LED_ERR 7
#define CMD_LEN (sizeof("T12345678811223344556677881234\r")+1)


// int g_can_speed = CANSPEED_500; // default: 500k
int g_ts_en = 0;
uint16_t ts = 0;

RH_RF95 rf95(LORA_PIN_NSS, LORA_PIN_INT);


// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_OPEN, OUTPUT);
  pinMode(LED_ERR, OUTPUT);

  // need to assert the output of this pin so the arduino will work in SPI master mode
  pinMode(SPI_PIN_ASSERT, OUTPUT);
  digitalWrite(SPI_PIN_ASSERT, 1);

  
  Serial.begin(INITIAL_BAUD);
  if (initLora(&rf95)) {
    digitalWrite(LED_OPEN, HIGH);
    digitalWrite(LED_ERR, LOW);
  } else {
    digitalWrite(LED_ERR, HIGH);
  }
}


void send_socketcan(tCAN *msg, int *i, char *p, char *buf) {
    p = buf;
    if (msg->header.ide) {
      if (msg->header.rtr) {
        sprintf(p, "R%04X%04X%01d", msg->id, msg->ide, msg->header.length);
      } else {
        sprintf(p, "T%04X%04X%01d", msg->id, msg->ide, msg->header.length);
      }
      p += 10;
    } else {
      if (msg->header.rtr) {
        sprintf(p, "r%03X%01X", msg->id, msg->header.length);
      } else {
        sprintf(p, "t%03X%01X", msg->id, msg->header.length);
      }
      p += 5;
    }
    for ((*i) = 0; (*i) < msg->header.length; (*i)++) {
      sprintf(p + ((*i) * 2), "%02X", msg->data[(*i)]);
    }
    p += (*i) * 2;

    // insert timestamp if needed
    if (g_ts_en) {
      sprintf(p, "%04X", ts++); // up to 60,000ms
      p += 4;
    }

    *(p++) = '\r';
    *(p++) = '\0';
    Serial.print(buf);
}


// transfer messages from CAN bus to host
void xfer_can2tty()
{
  tCAN msg;
  char buf[CMD_LEN];
  int i;
  char *p;

  while (recvFrame(&msg, &rf95)) {
    send_socketcan(&msg, &i, p, buf);
  }
}

void slcan_ack()
{
  Serial.write('\r'); // ACK
}

void slcan_nack()
{
  Serial.write('\a'); // NACK
}

void send_canmsg(char *buf)
{
  tCAN msg;
  int len = strlen(buf) - 1;
  int val, vale;
  int is_eff = buf[0] & 0x20 ? 0 : 1;
  int is_rtr = buf[0] & 0x02 ? 1 : 0;

  if (!is_eff && len >= 4) { // SFF
    sscanf(&buf[1], "%03x", &val);
    msg.id = val;
    msg.header.rtr = is_rtr;
    msg.header.ide = 0;
    sscanf(&buf[4], "%01x", &val);
    msg.header.length = val;
    if (len - 4 - 1 == msg.header.length * 2) {
      for (int i = 0; i < msg.header.length; i++) {
        sscanf(&buf[5 + (i * 2)], "%02x", &val);
        msg.data[i] = val;
      }
    }
    sendFrame(&msg, &rf95);

  } else if (is_eff && len >= 9) { // EFF
    sscanf(&buf[1], "%04x%04x", &val, &vale);
    msg.id = val & 0x1fff;
    msg.ide = vale;
    msg.header.rtr = is_rtr;
    msg.header.ide = 1;
    sscanf(&buf[9], "%01x", &val);
    msg.header.length = val;
    if (len - 9 - 1 == msg.header.length * 2) {
      for (int i = 0; i < msg.header.length; i++) {
        sscanf(&buf[10 + (i * 2)], "%02x", &val);
        msg.data[i] = val;
      }
    }
    sendFrame(&msg, &rf95);
  }
}

void pars_slcancmd(char *buf)
{
  switch (buf[0]) {
    // common commands
    case 'O': // open channel
      digitalWrite(LED_OPEN, HIGH);
      if (initLora(&rf95)) {
        digitalWrite(LED_ERR, LOW);
      } else {
        digitalWrite(LED_ERR, HIGH);
      }
      slcan_ack();
      break;
    case 'C': // close channel. Must reset baud!
      digitalWrite(LED_OPEN, LOW);
      digitalWrite(LED_ERR, LOW);
      slcan_ack();
      
      Serial.flush();
      Serial.begin(INITIAL_BAUD);
      while(!Serial)
        ;
        
      resetLora(&rf95);
      break;
    case 't': // SFF
    case 'T': // EFF
    case 'r': // RTR/SFF
    case 'R': // RTR/EFF
      send_canmsg(buf);
      slcan_ack();
      break;
    case 'Z': // turn timestamp on/off
      if (buf[1] == '0') {
        g_ts_en = 0;
      } else if (buf[1] == '1') {
        g_ts_en = 1;
      } else {
        slcan_nack();
      }
      slcan_ack();
      break;
    case 'M': // acceptance mask
      slcan_ack();
      break;
    case 'm': // acceptance value
      slcan_ack();
      break;

    // non-standard commands
    // Don't need speed setting commands as that is abstracted away by lora
    /*case 'S': // setup CAN bit-rates
      switch (buf[1]) {
        case '0': // 10k
        case '1': // 20k
        case '2': // 50k
          slcan_nack();
          break;
        case '3': // 100k
          g_can_speed = CANSPEED_100;
          slcan_ack();
          break;
        case '4': // 125k
          g_can_speed = CANSPEED_125;
          slcan_ack();
          break;
        case '5': // 250k
          g_can_speed = CANSPEED_250;
          slcan_ack();
          break;
        case '6': // 500k
          g_can_speed = CANSPEED_500;
          slcan_ack();
          break;
        case '7': // 800k
          slcan_nack();
          break;
        case '8': // 1000k
          g_can_speed = CANSPEED_1000;
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;*/
    case 's': // directly set bitrate register of mcp2515
      slcan_nack();
      break;
    case 'F': // status flag
      Serial.print("F12");
      slcan_ack();
      break;
    case 'V': // hw/sw version. When we get this, gotta shift baud to play nice with the CANdapter software
      Serial.print("V690420");
      slcan_ack();
      Serial.flush();
      Serial.begin(RUNNING_BAUD);
      while(!Serial)
        ;
      break;
    case 'N': // serial number
      Serial.print("N1337");
      slcan_ack();
      break;
    default: // unknown command
      slcan_nack();
      break;
  }
}

// transfer messages from host to CAN bus
void xfer_tty2can()
{
  int length;
  static char cmdbuf[CMD_LEN];
  static int cmdidx = 0;

  if ((length = Serial.available()) > 0) {
    for (int i = 0; i < length; i++) {
      char val = Serial.read();
      cmdbuf[cmdidx++] = val;

      if (cmdidx == CMD_LEN) { // command is too long
        slcan_nack();
        cmdidx = 0;
      } else if (val == '\r') { // end of command
        cmdbuf[cmdidx] = '\0';
        pars_slcancmd(cmdbuf);
        cmdidx = 0;
      }
    }
  }
}

// the loop function runs over and over again forever
void loop() {
  static uint32_t lastInfo = 0;
  
  xfer_can2tty();
  xfer_tty2can();
  
  if(millis() >= lastInfo + INFO_SEND_MILLIS) {
    tCAN infoMsg;
    if(getInfoFrame(&infoMsg, &rf95, INFO_FRAME_RECIEVER_ID)) {
      char buf[CMD_LEN];
      int i;
      char *p;
      
      // Send the info frame both to the socketcan client and the remote bus
      send_socketcan(&infoMsg, &i, p, buf);
      sendFrame(&infoMsg, &rf95);
      lastInfo = millis();
    }
  }
}

