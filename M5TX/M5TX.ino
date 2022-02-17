////////////////////////////////////////////////////////////////////////////////
// DIY-RCシステムのTX本体ソース
// DIY radio control system by M5Stack series
// https://github.com/hshin-git/M5RadioControl
////////////////////////////////////////////////////////////////////////////////


// Arduino M5Stack headder
#if defined(ARDUINO_M5Stick_C)
 #include <M5StickC.h>
#elif defined(ARDUINO_M5Stack_Core_ESP32)
 #define M5STACK_MPU6886 
 #include <M5Stack.h>
#elif defined(ARDUINO_M5STACK_Core2)
 #include <M5Core2.h>
#endif

// RC Library
#include "M5TX.hpp"
#include "M5Lib.hpp"


// PC conf
Preferences RC_PREF;
#define RC_MAGIC  12345678


// RC Link
ESP32NowRC RC_LINK;
M5_ADS1X15 AD_CONV;


// RC Data
TX2RX TX_DATA;  //TX->RX data
TX2Rx RX_CONF;  //TX->RX conf
RX2TX RX_DATA;  //RX->TX data


// TX Conf Counter
int TX_COUNT = 0;//TX->RX conf counter
#define TX_REPEAT 10

// TX Pairing Flag
int TX_PAIRING = false;

// RC Misc
float CHAN_ADC[RC_CHAN_MAX];
FilterLP CHAN_LPF[RC_CHAN_MAX];


// TX Task
UserTask TX_TASK;
UserTask UI_TASK;

// Freq counter
CountHZ RCV_FREQ;
CountHZ SND_FREQ;

// Vin Monitor
M5_AXP_VIN AXP_VIN;

// RTC
#if defined(_M5STACKC_H_) || defined(_M5CORE2_H_)
RTC_DateTypeDef RTC_DATE;
RTC_TimeTypeDef RTC_TIME;
#endif

// AHRS
M5StackAHRS TX_AHRS;
float IMU_pitch,IMU_roll,IMU_yaw;
#define IMU_CH1 (0.0 - (IMU_roll))
#define IMU_CH2 (0.0 - (IMU_pitch))


// SD LOGGER
#define BUF_SIZE  256
SDCServer SDC_SERV;
DataFILE<RX2TX> LOG_FILE(BUF_SIZE);
char *LOG_NAME(int peer) {
  static char buf[64];
  static int ses = -1;
  static int seq = 0;
  if (ses < 0) {
    RC_PREF.begin("LOG_NAME");
    ses = RC_PREF.getInt("session",0);
    RC_PREF.putInt("session",ses+1);
    RC_PREF.end();
  }
  sprintf(buf,"/b%02d-s%02d-p%02d-i%df%d.bin", (ses)%100,(seq++)%100, peer, 2,sizeof(RX2TX)/sizeof(float)-2);
  return buf;
}


// LCD conf
TFT_eSprite LCD_MENU = TFT_eSprite(&M5.Lcd);
TFT_eSprite LCD_PLOT = TFT_eSprite(&M5.Lcd);
#define BG_COLOR  TFT_BLACK
#define FG_COLOR  TFT_WHITE
#define FC_COLOR  TFT_DARKCYAN
#define ED_COLOR  TFT_YELLOW
#define TX_COLOR  TFT_YELLOW
#define RX_COLOR  (RCV_FREQ.getFreq()>0? TFT_GREEN: TFT_RED) 



////////////////////////////////////////////////////////////////////////////////
// TX_FACE: M5送信機のユーザーインターフェイス
//  setup(): 初期化
//  loop(): 定期実行
// 備考1: 送信機パラメータの永続化
//  当クラスのメンバ変数（static以外）は全て永続化対象
//  送信機パラメータはsetup()で初期化、パラメータ更新して保存する場合はsave()を呼ぶ
//  メニュー操作でパラメータ更新した場合、増減あり（dir!=0）かつBボタン押しでsave()を呼ぶ
// 備考2: スクロールメニューの追加方法
//  1.表示MENU()に追記する
//  2.処理UPDATE()に追記する
//  3.定数MENU_MAXを調整する（MENUのswitch文ラベルの最大値以上ならOK）
////////////////////////////////////////////////////////////////////////////////
class TX_FACE {
  #define NAME_SIZE  8
  #define NAME_CHAR  "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 "
public:
  // prefs
  int PEER; // 0 to 8
  int TRIM[RC_LINK_MAX][RC_CHAN_MAX]; // -100 to 100
  int RATE[RC_LINK_MAX][RC_CHAN_MAX]; // -100 to 100
  int NORM[RC_LINK_MAX][RC_CHAN_MAX]; // 0 to 1
  int FREQ[RC_LINK_MAX][RC_CHAN_MAX]; // 50 to 400
  int MEAN[RC_LINK_MAX][RC_CHAN_MAX]; // usec
  int MIN[RC_LINK_MAX][RC_CHAN_MAX];  // usec
  int MAX[RC_LINK_MAX][RC_CHAN_MAX];  // usec
  int EXP[RC_LINK_MAX][RC_CHAN_MAX];  // usec
  // gyro
  int AXIS[RC_LINK_MAX];  // 1 to 3
  int GPID[RC_LINK_MAX];  // 0 to 3
  int GAIN[RC_LINK_MAX][8]; // GPID x 2
  // name
  char NAME[RC_LINK_MAX][NAME_SIZE];
  // magic
  int MAGIC;
  // initialize
  void init() {
    PEER = 0;
    for (int n=0; n<RC_LINK_MAX; n++) {
      for (int ch=0; ch<RC_CHAN_MAX; ch++) {
        TRIM[n][ch] = 0;
        RATE[n][ch] = 0;
        NORM[n][ch] = 1;
        FREQ[n][ch] = 50;
        MEAN[n][ch] = 1500;
        MIN[n][ch] = 1000;
        MAX[n][ch] = 2000;
        EXP[n][ch] = 0;
      }
      AXIS[n] = 1;
      GPID[n] = 0;
      GAIN[n][0] = 0;
      GAIN[n][1] = 50;
      GAIN[n][2] = 0;
      GAIN[n][3] = 0;
      GAIN[n][4] = 0;
      GAIN[n][5] = 50;
      GAIN[n][6] = 0;
      GAIN[n][7] = 0;
      sprintf(NAME[n],"MODEL%02d",n);
    }
    MAGIC = RC_MAGIC;
  }
  void load() {
    if (RC_PREF.begin(RC_LINK_NAME)) {
      RC_PREF.getBytes("RC_PROP", (uint8_t*)this, sizeof(*this));
      RC_PREF.end();
    }
  }
  void save() {
    if (RC_PREF.begin(RC_LINK_NAME)) {
      RC_PREF.putBytes("RC_PROP", (uint8_t*)this, sizeof(*this));
      RC_PREF.end();
    }
  }
  void setup() {
    // prefs
    load();
    if (MAGIC != RC_MAGIC) {
      init();
      save();
    }
    // faces
    setup_LCD();
    // menus
    menu_init();
  }
  void setup_LCD() {
#if defined(_M5STICKC_H_)
    // M5StickC: 80 x 160
    #define TXT_SIZE  1
    #define TXT_FONT  1
    #define LCD_COLS  12
    #define LCD_ROWS  (20/TXT_SIZE)
    //
    M5.Lcd.setRotation(0);
    M5.Axp.ScreenBreath(10);
    M5.Lcd.fillScreen(BG_COLOR);
    M5.Lcd.setTextSize(1);
    // MENU Area
    LCD_MENU.createSprite(80,160);
    //LCD_MENU.createSprite(M5.Lcd.width(),M5.Lcd.height());
#elif defined(_M5STACK_H_) || defined(_M5Core2_H_)
    // M5Stack: 320 x 240
    #define TXT_SIZE  1
    #define TXT_FONT  1
    #define LCD_COLS  12
    #define LCD_ROWS  (30/TXT_SIZE)
    //M5.Lcd.setRotation(1);
    //M5.Lcd.setBrightness(128);
    M5.Lcd.fillScreen(BG_COLOR);
    // MENU Area
    LCD_MENU.setTextSize(TXT_SIZE);
    LCD_MENU.setColorDepth(8);
    LCD_MENU.createSprite(80,240);
    // PLOT Area
    LCD_PLOT.setTextSize(TXT_SIZE);
    LCD_PLOT.setColorDepth(8);
    LCD_PLOT.createSprite(240,240);
#endif
  }
  // RC function
  float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
  float map2usec(int ch, float pct) {
    if (ch == 0 && (dancing&0x1)) {
      pct = mapFloat(IMU_CH1, -45.0,45.0, 0.0,100.0);
      pct = constrain(pct, 0.0, 100.0);
    }
    if (ch == 1 && (dancing&0x2)) {
      pct = mapFloat(IMU_CH2, -45.0,45.0, 0.0,100.0);
      pct = constrain(pct, 0.0, 100.0);
    }
    pct = pct2exp(ch,pct);
    float trim = TRIM[PEER][ch]/1.0;
    float rate = RATE[PEER][ch]/200.0;
    float usec = mapFloat(NORM[PEER][ch]? pct: 100.0-pct, 0,100, 1000*(1.0-rate),1000*(2.0+rate)) + trim;
    return dir == 0? constrain(usec, MIN[PEER][ch],MAX[PEER][ch]): usec;
  }
  float pct2exp(int ch, float pct) {
    float f = constrain(EXP[PEER][ch] < 0? (100+EXP[PEER][ch])/100.0: (200+EXP[PEER][ch])/200.0, 0.0,1.5);  // from 0.0 to 1.5
    float x = (pct - 50.0) / 50.0;
    float y = ((1.0 - f)*x*x + f)*x;
    return 50.0 * (1.0 + y);
  }
  // MENU function
  #define MENU_MAX  70
  #define MENU_MIN  7
  #define MENU_LEN  (LCD_ROWS - MENU_MIN - 3)
  static int pos;
  static int dir;
  static int pos2num[MENU_MAX];
  static int pos_max;
  //
  static int dancing;
  static int logging;
  //
  void HEAD(const char *txt) {
    LCD_MENU.fillScreen(BG_COLOR);
    LCD_MENU.setCursor(0,0);
    LCD_MENU.setTextColor(BG_COLOR,FG_COLOR);
    //LCD_MENU.printf(" %-12s\n", txt);
    //LCD_MENU.printf(" %-7s%02d:%02d\n", txt,RTC_TIME.Hours,RTC_TIME.Minutes);
    if (TX_PAIRING) {
      LCD_MENU.printf(" #%d pairing..\n", PEER);
    } else {
      //LCD_MENU.printf(" #%d %3d/%3dHz\n", PEER,SND_FREQ.getFreq(),RCV_FREQ.getFreq());
      LCD_MENU.printf(" #%d %-9s\n", PEER,NAME[PEER]);
    }
    LCD_MENU.setTextColor(FG_COLOR,BG_COLOR);
  }
  void LINE(const char* txt) {
    LCD_MENU.setTextColor(BG_COLOR,FG_COLOR);
    LCD_MENU.printf(" %-12s\n", txt);
    LCD_MENU.setTextColor(FG_COLOR,BG_COLOR);
  }
  void FOOT(const char* txt) {
    LCD_MENU.setTextColor(BG_COLOR,FG_COLOR);
    //LCD_MENU.printf(" %-12s\n", txt);
    //LCD_MENU.printf(" %-7s%02d/%02d\n", txt,RTC_DATE.Month,RTC_DATE.Date);
#if defined(_M5STICKC_H_)
    if (dir == 0) {
      LCD_MENU.printf("%13s\n", "SELECT/ NEXT ");
    } else {
      LCD_MENU.printf("%13s\n", "UPDATE/ (+-=)");
    }
#elif defined(_M5STACK_H_) || defined(_M5Core2_H_)
    if (dir == 0) {
      LCD_MENU.printf("%13s\n", " PRV/SET/NXT ");
    } else {
      LCD_MENU.printf("%13s\n", " (-)/EXT/(+) ");
    }
#else
    LCD_MENU.printf("%13s\n", "");
#endif
    LCD_MENU.setTextColor(FG_COLOR,BG_COLOR);
    LCD_MENU.pushSprite(0,0);
  }
  void PLOT() {
    LCD_PLOT.fillScreen(BG_COLOR);
    // axis
    for (int t = -100; t<=100; t+=10) {
      int x = map(t, -100,100, 0,240);
      int y = map(t, -100,100, 240,0);
      LCD_PLOT.drawLine(x,0, x,240, t%50? TFT_DARKGREY: TFT_LIGHTGREY);
      LCD_PLOT.drawLine(0,y, 240,y, t%50? TFT_DARKGREY: TFT_LIGHTGREY);
    }
    // Radius of circles
    int R1 = 10;
    int R2 = 60;
    // TX channel
    int x0 = map(100.-CHAN_ADC[0], 0,100, 0,240);
    int y0 = map(0.00+CHAN_ADC[1], 0,100, 240,0);
    LCD_PLOT.drawLine(x0,0,x0,240, TX_COLOR);
    LCD_PLOT.drawLine(0,y0,240,y0, TX_COLOR);
    LCD_PLOT.fillCircle(x0,y0,R1/2, TX_COLOR);
    // TX roll/pitch
    int x1 = map(-IMU_CH1, -100,100, 0,240);
    int y1 = map( IMU_CH2, -100,100, 240,0);
    LCD_PLOT.fillCircle(x1,y1,R1, TX_COLOR);
    if (!isnan(RX_DATA.ahrs[0]) && RCV_FREQ.getFreq()>0) {
      // RX roll/pitch
      int x2 = map(RX_DATA.ahrs[1], -100,100, 0,240);
      int y2 = map(RX_DATA.ahrs[0], -100,100, 240,0);
      LCD_PLOT.fillCircle(x2,y2,R1, RX_COLOR);
      // RX accl
      int ax = RX_DATA.accl[0] * (R2/1.0);
      int ay = RX_DATA.accl[1] * (R2/1.0);
      LCD_PLOT.drawLine(x2,y2,x2+ax,y2-ay, RX_COLOR);
      LCD_PLOT.fillCircle(x2+ax,y2-ay,R1/2, RX_COLOR);
      // RX gyro
      float th = RX_DATA.gyro[2] * (PI/180.0);
      int wx = R2*sin(th);
      int wy = R2*cos(th);
      LCD_PLOT.drawCircle(x2,y2,R2, RX_COLOR);
      LCD_PLOT.drawLine(x2,y2,x2+0,y2-R2, RX_COLOR);
      LCD_PLOT.drawLine(x2,y2,x2-wx,y2-wy, RX_COLOR);
    } else {
      // RX roll/pitch
      int x2 = map(0, -100,100, 0,240);
      int y2 = map(0, -100,100, 240,0);
      LCD_PLOT.fillCircle(x2,y2,R1, RX_COLOR); 
    }
    // text
    LCD_PLOT.setTextColor(BG_COLOR,FG_COLOR);
    LCD_PLOT.setCursor(0,0);
    LCD_PLOT.printf("%-40s\n"," ");
    if (!isnan(RX_DATA.ahrs[0]) && RCV_FREQ.getFreq()>0) {
      LCD_PLOT.setCursor(0,0);
      LCD_PLOT.printf(" accl:%4.1fG/%4.1fG/%4.1fG gyro:%6.1fdps", RX_DATA.accl[0],RX_DATA.accl[1],RX_DATA.accl[2],RX_DATA.gyro[2]);
    }
    LCD_PLOT.setCursor(0,8*(LCD_ROWS-1));
    LCD_PLOT.printf("%-40s\n"," ");
    LCD_PLOT.pushSprite(80,0);
  }
  const char* CURSOR(int n) {
    if (pos2num[pos] == n) {
      LCD_MENU.setTextColor(dir?ED_COLOR:FG_COLOR, FC_COLOR);
      if (dir == 0) return "|";
      if (dir == 1) return "+";
      if (dir ==-1) return "-";
      if (dir == 2) return "=";
    }
    LCD_MENU.setTextColor(FG_COLOR,BG_COLOR);
    return " ";
  }
  // menu definition
  bool MENU(int n) {
    switch(n) {
      default: LCD_MENU.println(""); return false;
      // Info
      case  0: LCD_MENU.printf("%s%s\n",          CURSOR(n), "CH1/2 (us)"); break;
      case  1: LCD_MENU.printf("%s %5.0f/%5.0f\n",CURSOR(n), TX_DATA.usec[0],TX_DATA.usec[1]); break;
      case  2: LCD_MENU.printf("%s%s\n",          CURSOR(n), "AHRS (deg)"); break;
      case  3: LCD_MENU.printf("%s %5.1f/%5.1f\n",CURSOR(n), IMU_roll,IMU_pitch); break;
      case  4: LCD_MENU.printf("%s %5.1f/%5.1f\n",CURSOR(n), RX_DATA.ahrs[1],RX_DATA.ahrs[0]); break;
      case  5: LCD_MENU.printf("%s%s\n",          CURSOR(n), "TX/RX (Hz)"); break;
      case  6: LCD_MENU.printf("%s %5d/%5d\n",    CURSOR(n), SND_FREQ.getFreq(),RCV_FREQ.getFreq()); break;
      // Model
      case 10: LCD_MENU.printf("%s%s\n",          CURSOR(n), "Model"); break;
      case 11: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "peer",PEER); break;
      case 12: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "axis",AXIS[PEER]); break;
      // CH1
      case 20: LCD_MENU.printf("%s%s\n",          CURSOR(n), "CH1"); break;
      case 21: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "trim",TRIM[PEER][0]); break;
      case 22: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "rate",RATE[PEER][0]); break;
      case 23: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "norm",NORM[PEER][0]); break;
      case 24: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "freq",FREQ[PEER][0]); break;
      case 25: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "mean",MEAN[PEER][0]); break;
      case 26: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "min",MIN[PEER][0]); break;
      case 27: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "max",MAX[PEER][0]); break;
      case 28: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "exp",EXP[PEER][0]); break;
      // CH2
      case 30: LCD_MENU.printf("%s%s\n",          CURSOR(n), "CH2"); break;
      case 31: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "trim",TRIM[PEER][1]); break;
      case 32: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "rate",RATE[PEER][1]); break;
      case 33: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "norm",NORM[PEER][1]); break;
      case 34: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "freq",FREQ[PEER][1]); break;
      case 35: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "mean",MEAN[PEER][1]); break;
      case 36: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "min",MIN[PEER][1]); break;
      case 37: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "max",MAX[PEER][1]); break;
      case 38: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "exp",EXP[PEER][1]); break;
      // IMU1
      case 40: LCD_MENU.printf("%s%s\n",          CURSOR(n), "IMU1"); break;
      case 41: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "ON",GPID[PEER]&0x1); break;
      case 42: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KG",GAIN[PEER][0]); break;
      case 43: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KP",GAIN[PEER][1]); break;
      case 44: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KI",GAIN[PEER][2]); break;
      case 45: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KD",GAIN[PEER][3]); break;
#if 0
      // IMU2
      case 50: LCD_MENU.printf("%s%s\n",          CURSOR(n), "IMU2"); break;
      case 51: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "ON",GPID[PEER]&0x2); break;
      case 52: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KG",GAIN[PEER][4]); break;
      case 53: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KP",GAIN[PEER][5]); break;
      case 54: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KI",GAIN[PEER][6]); break;
      case 55: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "KD",GAIN[PEER][7]); break;
#else
      // ESC2
      case 50: LCD_MENU.printf("%s%s\n",          CURSOR(n), "ESC2"); break;
      case 51: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "ON",GPID[PEER]&0x2); break;
      case 52: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "BK",GAIN[PEER][4]); break;
      case 53: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "HZ",GAIN[PEER][5]); break;
      case 54: LCD_MENU.printf("%s %5s:%5d\n",    CURSOR(n), "MS",GAIN[PEER][6]); break;
#endif
      // CALL
      case 60: LCD_MENU.printf("%s%s\n",          CURSOR(n), "Call"); break;
      case 61: LCD_MENU.printf("%s %s: #%d\n",    CURSOR(n), "pairing",PEER); break;
      case 62: LCD_MENU.printf("%s %s: %2d\n",    CURSOR(n), "dance#1",dancing); break;
      case 63: LCD_MENU.printf("%s %s: %2d\n",    CURSOR(n), "dance#2",dancing); break;
      case 64: LCD_MENU.printf("%s %s: %2d\n",    CURSOR(n), "sdlog#1",logging); break;
      case 65: LCD_MENU.printf("%s %s: %2d\n",    CURSOR(n), "sdlog#2",logging); break;
      case 66: LCD_MENU.printf("%s %s:%.6s\n",    CURSOR(n), "name",NAME[PEER]); break;
    }
    return true;
  }
  // menu operation
  void UPDATE(int n) {
    switch(n) {
      default: dir = 0; break;
      // Model
      case 10: dir = 0; break;
      case 11: PEER = (PEER + dir + RC_LINK_MAX) % RC_LINK_MAX; TX_PAIRING = -1; break;
      case 12:
        if (dir) {
          if (AXIS[PEER] == -3) AXIS[PEER] = 1;
          else AXIS[PEER] = (AXIS[PEER]>0? -AXIS[PEER]: abs(AXIS[PEER]) + 1);
        }
        break;
      // CH1
      case 20: dir = 0; break;
      case 21: TRIM[PEER][0] = constrain(TRIM[PEER][0] + dir, -100,100); break;
      case 22: RATE[PEER][0] = constrain(RATE[PEER][0] + dir, -100,100); break;
      case 23: NORM[PEER][0] = NORM[PEER][0] ^ 0x1; break;
      case 24: FREQ[PEER][0] = constrain(FREQ[PEER][0] + 50*dir, 50,400); break;
      case 25:
      case 35: /* mean ch1-2 */
        for (int i=0; i<RC_CHAN_MAX; i++) MEAN[PEER][i] = TX_DATA.usec[i];
        break;
      case 26:
      case 27: /* min/max ch1 */
        if (TX_DATA.usec[0]<1400.) MIN[PEER][0] = TX_DATA.usec[0];
        else if (TX_DATA.usec[0]>1600.) MAX[PEER][0] = TX_DATA.usec[0];
        break;
      case 28: EXP[PEER][0] = constrain(EXP[PEER][0] + dir, -100,100); break;
      // CH2
      case 30: dir = 0; break;
      case 31: TRIM[PEER][1] = constrain(TRIM[PEER][1] + dir, -100,100); break;
      case 32: RATE[PEER][1] = constrain(RATE[PEER][1] + dir, -100,100); break;
      case 33: NORM[PEER][1] = NORM[PEER][1] ^ 0x1; break;
      case 34: FREQ[PEER][1] = constrain(FREQ[PEER][1] + 50*dir, 50,400); break;
      case 36:
      case 37: /* min/max ch2 */
        if (TX_DATA.usec[1]<1400.) MIN[PEER][1] = TX_DATA.usec[1];
        else if (TX_DATA.usec[1]>1600.) MAX[PEER][1] = TX_DATA.usec[1];
        break;
      case 38: EXP[PEER][1] = constrain(EXP[PEER][1] + dir, -100,100); break;
      // IMU1
      case 40: dir = 0; break;
      case 41: GPID[PEER] = GPID[PEER] ^ 0x1; break;
      case 42: GAIN[PEER][0] = constrain(GAIN[PEER][0] + dir, -100,100); break;
      case 43: GAIN[PEER][1] = constrain(GAIN[PEER][1] + dir, 0,100); break;
      case 44: GAIN[PEER][2] = constrain(GAIN[PEER][2] + dir, 0,100); break;
      case 45: GAIN[PEER][3] = constrain(GAIN[PEER][3] + dir, 0,100); break;
#if 0
      // IMU2
      case 50: dir = 0; break;
      case 51: GPID[PEER] = GPID[PEER] ^ 0x2; break;
      case 52: GAIN[PEER][4] = constrain(GAIN[PEER][4] + dir, -100,100); break;
      case 53: GAIN[PEER][5] = constrain(GAIN[PEER][5] + dir, 0,100); break;
      case 54: GAIN[PEER][6] = constrain(GAIN[PEER][6] + dir, 0,100); break;
      case 55: GAIN[PEER][7] = constrain(GAIN[PEER][7] + dir, 0,100); break;
#else
      // ESC2
      case 50: dir = 0; break;
      case 51: GPID[PEER] = GPID[PEER] ^ 0x2; break;
      case 52: GAIN[PEER][4] = constrain(GAIN[PEER][4] + dir, 0,1); break;
      case 53: GAIN[PEER][5] = constrain(GAIN[PEER][5] + 100*dir, 100,20*1000); break;
      case 54: GAIN[PEER][6] = constrain(GAIN[PEER][6] + 100*dir,   0,10*1000); break;
#endif
      // CALL
      case 60: dir = 0; break;
      case 61: dir = 0; TX_PAIRING = 1; break;
      case 62: dir = 0; dancing = dancing ^ 0x1; break;
      case 63: dir = 0; dancing = dancing ^ 0x2; break;
      case 64: dir = 0; if (!(logging&0x2)) { logging = logging ^ 0x1; if (logging&0x1) LOG_FILE.open(LOG_NAME(PEER)); else LOG_FILE.close(); }; break;
      case 65: dir = 0; if (!(logging&0x1)) { logging = logging ^ 0x2; if (logging&0x2) SDC_SERV.setup(); else SDC_SERV.stop(); }; break;
      case 66: EDIT(NAME[PEER]); break;
    }
  }
  void BUTTON() {
    M5.update();
#if defined(_M5STICKC_H_)
    // dir == 0: [A] select [B] next
    // dir != 0: [A] exit   [B] direction
    if (dir == 0 && M5.BtnA.wasPressed()) {
      dir = 2;
    }
    else if (dir == 0 && (M5.BtnB.wasPressed() || M5.BtnB.pressedFor(500))) {
      pos = constrain((pos + 1) % pos_max, MENU_MIN,pos_max);
    }
    else if (dir == 2 && M5.BtnA.wasPressed()) {
      dir = 0; save(); TX_COUNT = TX_REPEAT;
    }
    else if (abs(dir) == 1 && (M5.BtnA.wasPressed() || M5.BtnA.pressedFor(500))) {
      UPDATE(pos2num[pos]);
    }
    else if (dir != 0 && M5.BtnB.wasPressed()) {
      if (dir == 1) { dir = -1; }
      else if (dir == -1) { dir = 2; }
      else if (dir == 2) { dir = 1; }
    }
#elif defined(_M5STACK_H_) || defined(_M5Core2_H_)
    // dir == 0: [A] prev  [B] select [C] next
    // dir != 0: [A] minus [B] exit   [C] plus
    if (dir == 0 && M5.BtnB.wasPressed()) {
      dir = 2;
    }
    else if (dir == 0 && (M5.BtnA.wasPressed() || M5.BtnA.pressedFor(500))) {
      if (pos == MENU_MIN) pos = pos_max;
      pos = constrain((pos - 1) % pos_max, MENU_MIN,pos_max);
    }
    else if (dir == 0 && (M5.BtnC.wasPressed() || M5.BtnC.pressedFor(500))) {
      pos = constrain((pos + 1) % pos_max, MENU_MIN,pos_max);
    }
    else if (dir != 0 && M5.BtnB.wasPressed()) {
      dir = 0; UPDATE(pos2num[pos]); save(); TX_COUNT = TX_REPEAT;
    }
    else if (dir != 0 && (M5.BtnA.wasPressed() || M5.BtnA.pressedFor(500)))  {
      dir = -1; UPDATE(pos2num[pos]);
    }
    else if (dir != 0 && (M5.BtnC.wasPressed() || M5.BtnC.pressedFor(500)))  {
      dir = 1; UPDATE(pos2num[pos]);
    }
#endif
  }
  void EDIT(char* name) {
    const char* C = NAME_CHAR;
    const int N = sizeof(NAME_CHAR) - 1;
    int p = 0;
    int n = 0;
    while (true) {
      M5.update();
#if defined(_M5STICKC_H)
      if (M5.BtnA.wasPressed()) {
        if (p + 2 < NAME_SIZE) { p = p + 1; } 
        else { dir = 2; return; }
      } else
      if (M5.BtnB.wasPressed() || M5.BtnC.pressedFor(500)) {
        n = (n + 1) % N;
      }
#elif defined(_M5STACK_H_) || defined(_M5Core2_H_)
      if (M5.BtnA.wasPressed() || M5.BtnA.pressedFor(500)) {
        n = (n - 1 + N) % N;
      } else
      if (M5.BtnB.wasPressed()) {
        if (p + 2 < NAME_SIZE) { p = p + 1; }
        else { dir = 0; return; }
      } else
      if (M5.BtnC.wasPressed() || M5.BtnC.pressedFor(500)) {
        n = (n + 1) % N;
      }
#endif
      name[p] = C[n];
      M5.Lcd.setTextColor(ED_COLOR,FC_COLOR);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.printf(" #%d %-9s\n", PEER,name);
      delay(100);
    }
  }
  void loop() {
    // menu
    HEAD("M5-EXPEC");
    for (int n=0; n<MENU_MIN; n++) MENU(pos2num[n]);
    LINE("VARIABLE");
    for (int l=0; l<MENU_LEN; l++) {
      int n = pos<MENU_MIN+(MENU_LEN/2)? MENU_MIN+l: (pos-(MENU_LEN/2))+l;
      MENU(pos2num[n]);
    }
    FOOT("");
    // plot
#if defined(_M5STACK_H_) || defined(_M5Core2_H_)
    PLOT();
#endif
    // button
    BUTTON();
    // logger
    if (logging&0x1) LOG_FILE.flush(BUF_SIZE/2);
    if (logging&0x2) SDC_SERV.loop();
  }
  // initialize menu
  void menu_init() {
    for (int n=0; n<MENU_MAX; n++) pos2num[n] = -1;
    int p = 0;
    for (int n=0; n<MENU_MAX; n++) {
      if (MENU(n)) pos2num[p++] = n;
    }
    pos_max = p;
  }

} RC_CONF;
//
int TX_FACE::pos = MENU_MIN;
int TX_FACE::dir = 0;
int TX_FACE::pos2num[MENU_MAX];
int TX_FACE::pos_max = 0;
//
int TX_FACE::dancing = 0;
int TX_FACE::logging = 0;


// callback when data is sent from Master to Slave
unsigned long SentCount = 0;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  SentCount++;
  SND_FREQ.touch();
}


// callback when data is recv from Slave
unsigned long RecvCount = 0;
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (memcmp(mac_addr,RC_LINK.peerTX(RC_CONF.PEER),6)!=0) return;
  memcpy((uint8_t*)&RX_DATA,data,sizeof(RX_DATA));
  RecvCount++;
  RCV_FREQ.touch();
  int ack = *(int*)data;
  if (ack > 0) {
    TX_COUNT = 0;
    DEBUG.println("recv ACK");
  } else
  if (ack < 0) {
    TX_COUNT = TX_REPEAT;
    DEBUG.println("recv REQ");
  }
  else {
    // LOGGER
    if (RC_CONF.logging&0x1) LOG_FILE.write(&RX_DATA);
  }
}


// the setup
void setup()
{
  M5.begin();
#if defined(_M5STACK_H_) || defined(_M5Core2_H)
  M5.Power.begin();
  M5.Power.setPowerVin(false);
#endif
  M5.IMU.Init();
  M5.Lcd.println("M5TX@setup");

  // AHRS
  TX_AHRS.setup();

  // ADC
  AD_CONV.setup();

  // RC Conf
  RC_CONF.setup();
  
  // RC Link
  RC_LINK.setupTX();
  RC_LINK.pairingTX(RC_CONF.PEER);
  
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  TX_COUNT = TX_REPEAT;

  // RC Task
  UI_TASK.setup(loopUI,50,1,4096*2);
#if defined(_M5STICKC_H_) || defined(_M5Core2_H_)
  TX_TASK.setup(loopTX,1,1,4096*1);
#elif defined(_M5STACK_H_)
  TX_TASK.setup(loopTX,1,10,4096);
#endif

}


void loopTX()
{
  // IMU
  static float AHRS[3];
  TX_AHRS.loop(0,0,AHRS,0);
  IMU_pitch = AHRS[0];
  IMU_roll = AHRS[1];
  //DEBUG.printf("%f %f %f %f\n",AHRS[0],AHRS[1],IMU_pitch,IMU_roll);

  // ADC
  static float CHAN[RC_CHAN_MAX];
  AD_CONV.getPercents(CHAN,RC_CHAN_MAX);
  for (int i=0; i<RC_CHAN_MAX; i++) {
    //TX_DATA.usec[i] = RC_CONF.map2usec(i, AD_CONV.getPercent(i));
    //CHAN[i] = CHAN_LPF[i].update(CHAN[i]);
    if (i==0) CHAN_ADC[0] = RC_CONF.mapFloat(CHAN[0], 10.,90., 0.,100.);
    if (i==1) CHAN_ADC[1] = RC_CONF.mapFloat(CHAN[1], 25.,75., 0.,100.);
    TX_DATA.usec[i] = RC_CONF.map2usec(i, CHAN_ADC[i]);
  }
  
  // RC TX
  if (TX_COUNT > 0) {
    // 設定パケット
    TX_COUNT--;
    // conf
    RX_CONF.conf = 1;
    // CH settings
    for (int i=0; i<RC_CHAN_MAX; i++) RX_CONF.freq[i] = RC_CONF.FREQ[RC_CONF.PEER][i];
    for (int i=0; i<RC_CHAN_MAX; i++) RX_CONF.mean[i] = RC_CONF.MEAN[RC_CONF.PEER][i];
    for (int i=0; i<RC_CHAN_MAX; i++) RX_CONF.mins[i] = RC_CONF.MIN[RC_CONF.PEER][i];
    for (int i=0; i<RC_CHAN_MAX; i++) RX_CONF.maxs[i] = RC_CONF.MAX[RC_CONF.PEER][i];
    // IMU settings
    RX_CONF.axis = RC_CONF.AXIS[RC_CONF.PEER];
    RX_CONF.gpid = RC_CONF.GPID[RC_CONF.PEER];
    // steering assist PID gains
    RX_CONF.gain[0] = RC_CONF.GAIN[RC_CONF.PEER][0]/50. * (500./180.); // 180dps @ KG=50
    RX_CONF.gain[1] = RC_CONF.GAIN[RC_CONF.PEER][1]/50.;
    //RX_CONF.gain[2] = RC_CONF.GAIN[RC_CONF.PEER][2]/50.;
    //RX_CONF.gain[3] = RC_CONF.GAIN[RC_CONF.PEER][3]/50.;
    RX_CONF.gain[2] = RC_CONF.GAIN[RC_CONF.PEER][2]/250.;
    RX_CONF.gain[3] = RC_CONF.GAIN[RC_CONF.PEER][3]/5000.;
#if 0
    // throttle assist PID gains
    RX_CONF.gain[4] = RC_CONF.GAIN[RC_CONF.PEER][4]/50. * (500./5.0);  // 5.0G @ KG=50
    RX_CONF.gain[5] = RC_CONF.GAIN[RC_CONF.PEER][5]/50.;
    RX_CONF.gain[6] = RC_CONF.GAIN[RC_CONF.PEER][6]/500.;
    RX_CONF.gain[7] = RC_CONF.GAIN[RC_CONF.PEER][7]/500.;
#else
    // ESC parameters
    RX_CONF.gain[4] = RC_CONF.GAIN[RC_CONF.PEER][4];  // brake mode
    RX_CONF.gain[5] = RC_CONF.GAIN[RC_CONF.PEER][5];  // drive frequency in Hz
    RX_CONF.gain[6] = RC_CONF.GAIN[RC_CONF.PEER][6];  // drive frequency ramp in msec
    RX_CONF.gain[7] = 0.0;  // not in use
#endif
    //RX_CONF.debug();
    esp_now_send(RC_LINK.peerTX(RC_CONF.PEER), (uint8_t*)&RX_CONF, sizeof(RX_CONF));
    delay(100);
    DEBUG.println("send CONF");
  } else {
    // 制御パケット
    TX_DATA.conf = 0;
    esp_now_send(RC_LINK.peerTX(RC_CONF.PEER), (uint8_t*)&TX_DATA, sizeof(TX_DATA));
  }

  // RC TX Pairing
  if (TX_PAIRING != 0) {
    RC_LINK.pairingTX(RC_CONF.PEER, TX_PAIRING > 0);
    TX_PAIRING = 0;
  }

  // RTC
#if defined(_M5STACKC_H_) || defined(_M5CORE2_H_)
  M5.Rtc.GetTime(&RTC_TIME);
  M5.Rtc.GetData(&RTC_DATE);
#endif

  // VIN
  AXP_VIN.watch();

  // LOGGER
  //if (RC_CONF.logging&0x1) LOG_FILE.write(&RX_DATA);

}

void loopUI()
{
  // GUI
  RC_CONF.loop();
}

// the loop
void loop() { delay(1000); }
