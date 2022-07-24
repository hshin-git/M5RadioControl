////////////////////////////////////////////////////////////////////////////////
// DIY-RCシステムのRX本体ソース
// DIY radio control system by M5Stack series
// https://github.com/hshin-git/M5RadioControl
////////////////////////////////////////////////////////////////////////////////


// Arduino M5Stack Headder
#if defined(ARDUINO_M5Stack_ATOM)
#include <M5Atom.h>
#elif defined(ARDUINO_M5Stick_C)
#include <M5StickC.h>
#endif


// RC Library
#include "M5RX.hpp"
#include "M5Lib.hpp"


// RC Link
ESP32NowRC RC_LINK;


// RC Data
TX2RX TX_DATA;  //TX->RX data
TX2Rx RX_CONF;  //TX->RX conf
RX2TX RX_DATA;  //RX->TX data


// RX Conf Counter
int RX_COUNT = 0;


// Vin Monitor
M5_AXP_VIN AXP_VIN;


// PWM for CH1 and CH2
PulsePort PWM_OUT;

#if defined(_M5ATOM_H_)
const int PWM_PIN[] = {22,19,23,33};
const int GRV_PIN[] = {26,32};
#elif defined(_M5STICKC_H_)
const int PWM_PIN[] = {26, 0,32,33};
const int GRV_PIN[] = {32,33};
#endif

const int MAX_PWM = sizeof(PWM_PIN)/sizeof(int);
float USEC[RC_CHAN_MAX];


// PWM for ESC on Grove-connector
#if defined(_M5ATOM_H_)
//#define _M5ATOM_LED_  1
M5_DRV8833 ESC_PWM(GRV_PIN[0],GRV_PIN[1], 4,5);
//M5_DRV8833 ESC_PWM(PWM_PIN[2],PWM_PIN[3], 4,5);
#elif defined(_M5STICKC_H_)
M5_DRV8833 ESC_PWM(GRV_PIN[0],GRV_PIN[1], 4,5);
#endif


// PID
ServoPID CH1_PID;
ServoPID CH2_PID;

// Timer
CountHZ RCV_FREQ;
CountHZ SND_FREQ;
TimerMS WDT_MSEC;
TimerMS LCD_MSEC;

// AHRS
M5StackAHRS RX_AHRS;
float ACCL[3] = {0.0,0.0,0.0};
float GYRO[3] = {0.0,0.0,0.0};
float AHRS[3] = {0.0,0.0,0.0};


// RX LED
class RX_FACE {
public:
#if defined(_M5ATOM_H_)
  #define LED_NUM 25
  #define LED_PIN 27
  #define LED_POS(x,y)  (((x)+5*(y))%LED_NUM)
  void setup() {
    DEBUG.println("M5RX@setup");
    setNoLink();
  }
  #if defined(_M5ATOM_LED_)
  void setNoLink() {
    int p = LED_POS(2,2);
    M5.dis.fillpix(CRGB::Black);
    M5.dis.drawpix(p,CRGB::Red);
  }
  void setLinked() {
    int x = (int)map(TX_DATA.usec[0], 1000,2000, 0,4);
    int y = (int)map(TX_DATA.usec[1], 1000,2000, 0,4);
    int p = LED_POS(x,y);
    M5.dis.fillpix(CRGB::Black);
    M5.dis.drawpix(p,CRGB::Green);
  }
  void setPairing() {
    int p = LED_POS(2,2);
    M5.dis.fillpix(CRGB::Black);
    M5.dis.drawpix(p,CRGB::Yellow);
  }
  #else
  void setNoLink() {}
  void setLinked() {}
  void setPairing() {}
  #endif
#elif defined(_M5STICKC_H_)
  void setup() {
    M5.Axp.ScreenBreath(10);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.printf(" %-12s", "M5RX@setup");
    setNoLink();
  }
  void setNoLink() {
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextColor(TFT_BLACK,TFT_RED);
    M5.Lcd.printf(" %-12s", "NoLink");
  }
  void setLinked() {
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextColor(TFT_BLACK,TFT_GREEN);
    M5.Lcd.printf(" %-12s", "Linked");
  }
  void setPairing() {
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextColor(TFT_BLACK,TFT_YELLOW);
    M5.Lcd.printf(" %-12s", "Pairing");
  }
#endif
} RX_LEDS;


// callback when data is sent from Master to Slave
unsigned long SentCount = 0;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  SentCount++;
  SND_FREQ.touch();
}

// callback when data is recv from Slave
unsigned long RecvCount = 0;
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  RecvCount++;
  RCV_FREQ.touch();
  WDT_MSEC.touch();
  
  int conf = *(int*)data;
  if (conf) {
    RX_COUNT++;
    if (data_len == sizeof(RX_CONF)) memcpy((uint8_t*)&RX_CONF, data, sizeof(RX_CONF));
    DEBUG.println("recv CONF");
  } else {
    if (data_len == sizeof(TX_DATA)) memcpy((uint8_t*)&TX_DATA, data, sizeof(TX_DATA));
  }
}


/* After Atom-Matrix is started or reset
   the program in the setUp () function will be run, and this part will only be run once.
*/
void setup()
{
#if defined(_M5ATOM_H_)
  #if defined(_M5ATOM_LED_)
  M5.begin(true,true,true); //Init M5Atom-Matrix(Serial, I2C, LEDs).
  #else
  M5.begin(true,true,false); //Init M5Atom-Matrix(Serial, I2C, LEDs).
  #endif
#elif defined(_M5STICKC_H_)
  M5.begin();
#endif
  M5.IMU.Init();  //Init IMU sensor.

  // LED Face
  RX_LEDS.setup();

  // RC Link
  RC_LINK.setupRX();
  while (true) {
    M5.update();
    RX_LEDS.setPairing();
#if defined(_M5ATOM_H_)
    if (RC_LINK.pairingRX(M5.Btn.isPressed())) break;
#elif defined(_M5STICKC_H_)
    if (RC_LINK.pairingRX(M5.BtnA.isPressed())) break;
#endif
  }

  // IMU/AHRS
  RX_AHRS.setup();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // RC Conf
  RX_LEDS.setNoLink();
  while (true) {
    RX_DATA.ack = -1;
    esp_now_send(RC_LINK.peerRX(), (uint8_t*)&RX_DATA, sizeof(RX_DATA));
    if (RX_COUNT > 0) break;
    delay(20);
    DEBUG.println("send REQ");
  }

  // PWM Out
  for (int i=0; i<RC_CHAN_MAX; i++) {
    PWM_OUT.setupOut(PWM_PIN[i]);
  }
}

/* After the program in setup() runs, it runs the program in loop()
   The loop() function is an infinite loop in which the program runs repeatedly
 */
void loop()
{
  // IMU read
  RX_AHRS.loop(GYRO,ACCL,AHRS,0);

  // PWM Out
  for (int i=0; i<RC_CHAN_MAX; i++) {
    USEC[i] = TX_DATA.usec[i];
    // steering assist
    if (i==0 && (RX_CONF.gpid&0x1)) {
      float SP = USEC[0];
      float PV = RX_CONF.gain[0] * GYRO[2];
      //float PV = RX_CONF.gain[0] * RX_AHRS.getYawRate();
      USEC[0] = CH1_PID.loop(SP,PV);
    }
#if 0
    // throttle assist
    if (i==1 && (RX_CONF.gpid&0x2)) {
      float SP = USEC[1];
      float PV = RX_CONF.gain[4] * RX_AHRS.getAccL();
      USEC[1] = CH2_PID.loop(SP,PV);
      if (abs(TX_DATA.usec[1] - RX_CONF.mean[1]) < 500./20) USEC[1] = TX_DATA.usec[1];
    }
#else
    // drive ESC gate
    if (i==1 && (RX_CONF.gpid&0x2)) ESC_PWM.drive(WDT_MSEC.isOld(100)? 1500: USEC[i]);
#endif
    // pulse out
    PWM_OUT.putUsec(i, WDT_MSEC.isOld(100)? 0.0: USEC[i]);
  }

  // RC RX
  if (RX_COUNT > 0) {
    RX_COUNT--;
    RX_DATA.ack = 1;
    // conf
    for (int i=0; i<RC_CHAN_MAX; i++) PWM_OUT.putFreq(i, RX_CONF.freq[i]);
    CH1_PID.setup(RX_CONF.gain[1],RX_CONF.gain[2],RX_CONF.gain[3], RX_CONF.mins[0],RX_CONF.mean[0],RX_CONF.maxs[0],RX_CONF.freq[0]);
    CH2_PID.setup(RX_CONF.gain[5],RX_CONF.gain[6],RX_CONF.gain[7], RX_CONF.mins[1],RX_CONF.mean[1],RX_CONF.maxs[1],RX_CONF.freq[1]);
    ESC_PWM.setup(RX_CONF.gain[4]!=0,(int)RX_CONF.gain[5],(int)RX_CONF.gain[6]);
    RX_AHRS.initAXIS(RX_CONF.axis);
    DEBUG.println("send ACK");
  } else {
    RX_DATA.ack = 0;
  }
  //
  RX_DATA.msec = millis();
  for (int i=0; i<3; i++) {
    RX_DATA.accl[i] = ACCL[i];
    RX_DATA.gyro[i] = GYRO[i];
    RX_DATA.ahrs[i] = AHRS[i];
  }
  for (int i=0; i<RC_CHAN_MAX; i++) {
    RX_DATA.usec[i] = TX_DATA.usec[i];
    RX_DATA.sout[i] = USEC[i];
  }
  esp_now_send(RC_LINK.peerRX(), (uint8_t*)&RX_DATA, sizeof(RX_DATA));

  // WDT
  if (WDT_MSEC.isOld(100)) {
    RX_LEDS.setNoLink();
  } else {
    RX_LEDS.setLinked(); 
  }

  // VIN
  AXP_VIN.watch();

  // Delay
  delay(1);
}
