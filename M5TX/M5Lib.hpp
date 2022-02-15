////////////////////////////////////////////////////////////////////////////////
// DIY-RCシステムのTX/RX共通ソース
// DIY radio control system by M5Stack series
// https://github.com/hshin-git/M5RadioControl
////////////////////////////////////////////////////////////////////////////////
#ifndef _M5RC_LIB_H_
#define _M5RC_LIB_H_


//#define DEBUG M5.Lcd
#define DEBUG Serial


////////////////////////////////////////////////////////////////////////////////
// class TX2RX{}: 送信機から受信機へのパケット（制御データ）
////////////////////////////////////////////////////////////////////////////////
#define RC_CHAN_MAX 2

class TX2RX {
public:
  int conf; // config or not
  float usec[RC_CHAN_MAX];  // PWM in usec
  TX2RX() {
    conf = 0;
    for (int i=0; i<RC_CHAN_MAX; i++) usec[i] = 0.0;
  }
  int getSize() { return sizeof(*this); }
  void debug() {
    DEBUG.printf("TX2RX(%d)\n",getSize());
    DEBUG.printf(" conf:%d\n",conf);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" usec[%d]:%6.1f\n",i,usec[i]);
  }
};

////////////////////////////////////////////////////////////////////////////////
// class TX2Rx{}: 送信機から受信機へのパケット（設定データ）
////////////////////////////////////////////////////////////////////////////////
class TX2Rx {
public:
  int conf; // config or not
  int freq[RC_CHAN_MAX];  // PWM freq in Hz
  int mean[RC_CHAN_MAX];  // PWM mean in usec
  int mins[RC_CHAN_MAX];  // PWM min in usec
  int maxs[RC_CHAN_MAX];  // PWM max in usec
  int axis; // IMU axis
  int gpid; // PID flag
  float gain[8];  // PID gain (Kg,Kp,Ki,Kd)
  TX2Rx() {
    conf = 1;
    for (int i=0; i<RC_CHAN_MAX; i++) freq[i] = 50;
    for (int i=0; i<RC_CHAN_MAX; i++) mean[i] = 1500;
    for (int i=0; i<RC_CHAN_MAX; i++) mins[i] = 500;
    for (int i=0; i<RC_CHAN_MAX; i++) maxs[i] = 2000;
    axis = 1;
    gpid = 0;
    for (int i=0; i<8; i++) gain[i] = 0.0;
    gain[1] = 1.0;
    gain[5] = 1.0;
  }
  int getSize() { return sizeof(*this); }
  void debug() {
    DEBUG.printf("TX2Rx(%d)\n",getSize());
    DEBUG.printf(" conf:%d\n",conf);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" freq[%d]:%6d\n",i,freq[i]);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" mean[%d]:%6d\n",i,mean[i]);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" mins[%d]:%6d\n",i,mins[i]);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" maxs[%d]:%6d\n",i,maxs[i]);
    DEBUG.printf(" axis:%d\n",axis);
    DEBUG.printf(" gpid:%d\n",gpid);
    for (int i=0; i<8; i++) DEBUG.printf(" gain[%d]:%6.2f\n",i,gain[i]);
  }
};

////////////////////////////////////////////////////////////////////////////////
// class RX2TX{}: 受信機から送信機へのパケット（計測データ）
////////////////////////////////////////////////////////////////////////////////
class RX2TX {
public:
  int ack;  // ACK for config
  int msec; // lapse time in msec
  float accl[3];  // IMU accelerometer
  float gyro[3];  // IMU gyrometer
  float ahrs[3];  // AHRS pitch,roll,yaw
  float usec[RC_CHAN_MAX];  // PWM input
  float sout[RC_CHAN_MAX];  // PWM output
  float padding[1];
  RX2TX() {
    ack = 0;
    msec = 0;
    for (int i=0; i<3; i++) {
      accl[i] = 0.0;
      gyro[i] = 0.0;
      ahrs[i] = 0.0;
    }
    for (int i=0; i<RC_CHAN_MAX; i++) {
      usec[i] = 0.0;
      sout[i] = 0.0;
    }
  }
  int getSize() { return sizeof(*this); }
  void debug() {
    DEBUG.printf("RX2TX(%d)\n",getSize());
    DEBUG.printf(" ack:%d\n",ack);
    DEBUG.printf(" msec:%d\n",msec);
    for (int i=0; i<3; i++) DEBUG.printf(" accl[%d]:%6.1f\n",i,accl[i]);
    for (int i=0; i<3; i++) DEBUG.printf(" gyro[%d]:%6.1f\n",i,gyro[i]);
    for (int i=0; i<3; i++) DEBUG.printf(" ahrs[%d]:%6.1f\n",i,ahrs[i]);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" usec[%d]:%6.1f\n",i,usec[i]);
    for (int i=0; i<RC_CHAN_MAX; i++) DEBUG.printf(" sout[%d]:%6.1f\n",i,sout[i]);
  }
};




////////////////////////////////////////////////////////////////////////////////
// class ESP32NowRC{}: データ送受信用ライブラリ（ESP Now通信、ペアリング機能など）
//  setupTX(): 送信機TXの初期化
//  pairingTX(): 送信機TXのペアリング
//  setupRX(): 受信機RXの初期化
//  pairingRX(): 受信機RXのペアリング
////////////////////////////////////////////////////////////////////////////////
/**
   ESPNOW - Basic communication - Master/Slave
   Date: 26th September 2017
   Author: Arvind Ravulavaru <https://github.com/arvindr21>
   Purpose: ESPNow Communication between a Master ESP32 and a Slave ESP32
   Description: This sketch consists of the code for the Slave module.
   Resources: (A bit outdated)
   a. https://espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf
   b. http://www.esploradores.com/practica-6-conexion-esp-now/

   << This Device Slave >>

   Flow: Master
   Step 1 : ESPNow Init on Master and set it in STA mode
   Step 2 : Start scanning for Slave ESP32 (we have added a prefix of `slave` to the SSID of slave for an easy setup)
   Step 3 : Once found, add Slave as peer
   Step 4 : Register for send callback
   Step 5 : Start Transmitting data from Master to Slave

   Flow: Slave
   Step 1 : ESPNow Init on Slave
   Step 2 : Update the SSID of Slave with a prefix of `slave`
   Step 3 : Set Slave in AP mode
   Step 4 : Register for receive callback and wait for data
   Step 5 : Once data arrives, print it in the serial monitor

   Note: Master and Slave have been defined to easily understand the setup.
         Based on the ESPNOW API, there is no concept of Master and Slave.
         Any devices can act as master or salve.
*/
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>

/* RC Link paring parameter */
#define RC_LINK_NAME  "M5_RC_LINK"
#define RC_LINK_MSEC  (30*1000)
#define RC_LINK_MAX   8
#define RC_WIFI_CHANNEL 1

// Global copy of slave
//esp_now_peer_info_t slave;
#define PRINTSCANRESULTS 1
#define DELETEBEFOREPAIR 1


class ESP32NowRC {
private:
  Preferences pref;

  /* RC-TX: Master
   * https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/ESPNow/Basic/Master
   */
  static esp_now_peer_info_t slaveInfo[RC_LINK_MAX];
  void loadTX() {
    if (pref.begin(RC_LINK_NAME)) {
      pref.getBytes("slaveInfo",slaveInfo,sizeof(slaveInfo));
      pref.end();
    }
  }
  void saveTX() {
    if (pref.begin(RC_LINK_NAME)) {
      pref.putBytes("slaveInfo",slaveInfo,sizeof(slaveInfo));
      pref.end();
    }
  }
  
  // Init ESP Now with fallback
  void InitESPNow() {
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
      DEBUG.println("ESPNow Init Success");
    }
    else {
      DEBUG.println("ESPNow Init Failed");
      // Retry InitESPNow, add a counte and then restart?
      // InitESPNow();
      // or Simply Restart
      ESP.restart();
    }
  }
  
  // Scan for slaves in AP mode
  bool ScanForSlave(esp_now_peer_info_t *slave) {
    int8_t scanResults = WiFi.scanNetworks();
    // reset on each scan
    bool slaveFound = 0;
    memset(slave, 0, sizeof(*slave));
  
    DEBUG.println("");
    if (scanResults == 0) {
      DEBUG.println("No WiFi devices in AP Mode found");
    } else {
      DEBUG.print("Found "); DEBUG.print(scanResults); DEBUG.println(" devices ");
      for (int i = 0; i < scanResults; ++i) {
        // Print SSID and RSSI for each device found
        String SSID = WiFi.SSID(i);
        int32_t RSSI = WiFi.RSSI(i);
        String BSSIDstr = WiFi.BSSIDstr(i);
  
        if (PRINTSCANRESULTS) {
          DEBUG.print(i + 1);
          DEBUG.print(": ");
          DEBUG.print(SSID);
          DEBUG.print(" (");
          DEBUG.print(RSSI);
          DEBUG.print(")");
          DEBUG.println("");
        }
        delay(10);
        // Check if the current device starts with `Slave`
        if (SSID.indexOf("Slave") == 0) {
          // SSID of interest
          DEBUG.println("Found a Slave.");
          DEBUG.print(i + 1); DEBUG.print(": "); DEBUG.print(SSID); DEBUG.print(" ["); DEBUG.print(BSSIDstr); DEBUG.print("]"); DEBUG.print(" ("); DEBUG.print(RSSI); DEBUG.print(")"); DEBUG.println("");
          // Get BSSID => Mac Address of the Slave
          int mac[6];
          if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
            for (int ii = 0; ii < 6; ++ii ) {
              slave->peer_addr[ii] = (uint8_t) mac[ii];
            }
          }
  
          slave->channel = RC_WIFI_CHANNEL; // pick a channel
          slave->encrypt = 0; // no encryption
  
          slaveFound = 1;
          // we are planning to have only one slave in this example;
          // Hence, break after we find one, to be a bit efficient
          break;
        }
      }
    }
  
    if (slaveFound) {
      DEBUG.println("Slave Found, processing..");
    } else {
      DEBUG.println("Slave Not Found, trying again.");
    }
  
    // clean up ram
    WiFi.scanDelete();
  
    return slaveFound;
  }
  
  // Check if the slave is already paired with the master.
  // If not, pair the slave with master
  bool manageSlave(esp_now_peer_info_t *slave) {
    if (slave->channel == RC_WIFI_CHANNEL) {
      if (DELETEBEFOREPAIR) {
        deletePeer(slave);
      }
  
      DEBUG.print("Slave Status: ");
      // check if the peer exists
      bool exists = esp_now_is_peer_exist(slave->peer_addr);
      if ( exists) {
        // Slave already paired.
        DEBUG.println("Already Paired");
        return true;
      } else {
        // Slave not paired, attempt pair
        esp_err_t addStatus = esp_now_add_peer(slave);
        if (addStatus == ESP_OK) {
          // Pair success
          DEBUG.println("Pair success");
          return true;
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
          // How did we get so far!!
          DEBUG.println("ESPNOW Not Init");
          return false;
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
          DEBUG.println("Invalid Argument");
          return false;
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
          DEBUG.println("Peer list full");
          return false;
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
          DEBUG.println("Out of memory");
          return false;
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
          DEBUG.println("Peer Exists");
          return true;
        } else {
          DEBUG.println("Not sure what happened");
          return false;
        }
      }
    } else {
      // No slave found to process
      DEBUG.println("No Slave found to process");
      return false;
    }
  }
  
  void deletePeer(esp_now_peer_info_t *slave) {
    esp_err_t delStatus = esp_now_del_peer(slave->peer_addr);
    DEBUG.print("Slave Delete Status: ");
    if (delStatus == ESP_OK) {
      // Delete success
      DEBUG.println("Success");
    } else if (delStatus == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      DEBUG.println("ESPNOW Not Init");
    } else if (delStatus == ESP_ERR_ESPNOW_ARG) {
      DEBUG.println("Invalid Argument");
    } else if (delStatus == ESP_ERR_ESPNOW_NOT_FOUND) {
      DEBUG.println("Peer not found.");
    } else {
      DEBUG.println("Not sure what happened");
    }
  }

public:  
  void setupTX() {
    loadTX();
  //  M5.begin();
  //  DEBUG.begin(115200);
    //Set device in STA mode to begin with
    WiFi.mode(WIFI_STA);
    DEBUG.println("ESPNow/Basic/Master Example");
    // This is the mac address of the Master in Station Mode
    DEBUG.print("STA MAC: "); DEBUG.println(WiFi.macAddress());
    // Init ESPNow with a fallback logic
    InitESPNow();
    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
  //  esp_now_register_send_cb(OnDataSent);
  //  esp_now_register_recv_cb(OnDataRecv);
  }
  
  bool pairingTX(int id, bool scan = false) {
    esp_now_peer_info_t *slave;
  
    if (id>=0 && id<RC_LINK_MAX) {
      slave = &slaveInfo[id];
    } else {
      return false;
    }
    
    // In the loop we scan for slave
    if (scan) {
      unsigned int timeout = millis() + RC_LINK_MSEC;
      while (millis() < timeout) {
        esp_now_peer_info_t slave_addr;
        if (ScanForSlave(&slave_addr)) {
          memcpy(slave,&slave_addr,sizeof(slave_addr));
          saveTX();
          break;
        }
        delay(100);
      }
    }
    
    // If Slave is found, it would be populate in `slave` variable
    // We will check if `slave` is defined and then we proceed further
    bool isPaired = false;
    if (slave->channel == RC_WIFI_CHANNEL) { // check if slave channel is defined
      // `slave` is defined
      // Add slave as peer if it has not been added already
      isPaired = manageSlave(slave);
      if (isPaired) {
        // pair success or already paired
        // Send data to device
        //sendData();
      } else {
        // slave pair failed
        DEBUG.println("Slave pair failed!");
      }
    }
    else {
      // No slave found to process
    }
  
    // wait for 3seconds to run the logic again
    //delay(3000);
    return isPaired;
  }
  
private:
  /* RC-RX: Slave
   * https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/ESPNow/Basic/Slave
   */
  static esp_now_peer_info_t masterInfo;
  static bool masterFound;
  void loadRX() {
    if (pref.begin(RC_LINK_NAME)) {
      pref.getBytes("masterInfo",&masterInfo,sizeof(masterInfo));
      pref.end();
    }
  }
  void saveRX() {
    if (pref.begin(RC_LINK_NAME)) {
      pref.putBytes("masterInfo",&masterInfo,sizeof(masterInfo));
      pref.end();
    }
  }
  
  // config AP SSID
  void configDeviceAP() {
    const char *SSID = "Slave_1";
    bool result = WiFi.softAP(SSID, "Slave_1_Password", RC_WIFI_CHANNEL, 0);
    if (!result) {
      DEBUG.println("AP Config failed.");
    } else {
      DEBUG.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    }
  }
  
  static void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    if (!masterFound) {
      masterFound = true;
      memset(&masterInfo, 0, sizeof(masterInfo));
      memcpy(masterInfo.peer_addr, mac_addr, 6);
      masterInfo.channel = RC_WIFI_CHANNEL;
      masterInfo.encrypt = 0;
      //masterInfo.ifidx = ESP_IF_WIFI_AP;
      masterInfo.ifidx = WIFI_IF_AP;
    }
  }
  
public:
  void setupRX() {
    loadRX();
//  M5.begin(true, true, true);
//  DEBUG.begin(115200);
    DEBUG.println("ESPNow/Basic/Slave Example");
    //Set device in AP mode to begin with
    WiFi.mode(WIFI_AP);
    // configure device AP mode
    configDeviceAP();
    // This is the mac address of the Slave in AP Mode
    DEBUG.print("AP MAC: "); DEBUG.println(WiFi.softAPmacAddress());
    // Init ESPNow with a fallback logic
    InitESPNow();
    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info.
//  esp_now_register_recv_cb(OnDataRecv);
//  esp_now_register_send_cb(OnDataSent);
  }
  
  bool pairingRX(bool scan = false) {
    
    if (scan) {
      masterFound = false;
      esp_now_register_recv_cb(OnDataRecv);
      unsigned int timeout = millis() + RC_LINK_MSEC;
      while (millis() < timeout) {
        if (masterFound) {
          saveRX();
          break;
        }
        delay(100);
      }
      esp_now_unregister_recv_cb();
    }
  
    return esp_now_add_peer(&masterInfo) == ESP_OK;
  }
  
  uint8_t* peerTX(int id) { return slaveInfo[id].peer_addr; }
  uint8_t* peerRX() { return masterInfo.peer_addr; }

  const char* mac2str(const uint8_t* mac) {
    static char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return macStr;
  }

};

esp_now_peer_info_t ESP32NowRC::slaveInfo[RC_LINK_MAX];
esp_now_peer_info_t ESP32NowRC::masterInfo;
bool ESP32NowRC::masterFound;




////////////////////////////////////////////////////////////////////////////////
// class M5_AXP_VIN{}: 入力電圧モニタリング（M5StickC専用、Vin低下時にHalt）
//  watch(): 入力電圧Vinの監視（定期的に実行）
// https://lang-ship.com/blog/work/m5stickc-axp192-ext/
////////////////////////////////////////////////////////////////////////////////
#if defined(_M5STICKC_H_)
class M5_AXP_VIN {
  unsigned int lastVinTime = 0;
  void axp_halt(){
    Wire1.beginTransmission(0x34);
    Wire1.write(0x32);
    Wire1.endTransmission();
    Wire1.requestFrom(0x34, 1);
    uint8_t buf = Wire1.read();
    Wire1.beginTransmission(0x34);
    Wire1.write(0x32);
    Wire1.write(buf | 0x80); // halt bit
    Wire1.endTransmission();
  }
public:
  void watch() {
    float vin = M5.Axp.GetVinData()*1.7 /1000;
    float usb = M5.Axp.GetVusbinData()*1.7 /1000;
    //Serial.printf("vin,usb = %f,%f\n",vin,usb);
    if ( vin < 3.0 && usb < 3.0 ) {
      if ( lastVinTime + 5000 < millis() ) {
        axp_halt();
      }
    } else {
      lastVinTime = millis();
    }
  }
};
#else
class M5_AXP_VIN {
public:
  void watch() {}
};
#endif




////////////////////////////////////////////////////////////////////////////////
// class UserTask{}: タスク管理用ライブラリ（実行スレッドの制御）
//  setup(): タスク開始
//  suspend(): タスク中断
//  resume(): タスク再開
//  destroy(): タスク終了
//  getFreq(): ループ周期の取得[Hz]
////////////////////////////////////////////////////////////////////////////////
typedef struct {
  TaskHandle_t handle;
  void (*func)(void);
  int msec;
  bool flag;
  char name[8];
  uint32_t last, delta;
} TaskInfo;

class UserTask {
  #define MAX_TASK  4
  int task_id;
  static int task_count;
  static TaskInfo task_info[MAX_TASK];
  /* */
  static void taskLoop(void *args) {
    TaskInfo *info = &task_info[(int)args];
    while (true) {
      info->delta = micros() - info->last;
      info->last = micros();
      if (info->flag) (*(info->func))();
      delay(info->msec);
    }
  }
public:
  void setup(void (*func)(void), int msec=1, int prio=0, int stack=4096, int core=1) {
    if (task_count>=MAX_TASK) return;
    task_id = task_count++;
    TaskInfo *info = &task_info[task_id];
    info->func = func;
    info->msec = msec > 1? msec: 1;
    info->flag = true;
    info->last = 0;
    info->delta = 1;
    sprintf(info->name,"task%d",task_id);
    xTaskCreatePinnedToCore(taskLoop, info->name, stack, (void*)task_id, prio, &info->handle, core);
  }
  void suspend(void) {
    TaskInfo *info = &task_info[task_id];
    info->flag = false;
    delay(info->msec);
  }
  void resume(void) {
    TaskInfo *info = &task_info[task_id];
    info->flag = true;
    delay(info->msec);
  }
  void destroy(void) {
    TaskInfo *info = &task_info[task_id];
    vTaskDelete(info->handle);
  }
  int getFreq(void) {
    TaskInfo *info = &task_info[task_id];
    return info->flag? 1000000L/info->delta: 0;
  }
};
int UserTask::task_count = 0;
TaskInfo UserTask::task_info[MAX_TASK];




////////////////////////////////////////////////////////////////////////////////
// class TimerMS{}: タイマ管理用ライブラリ
//  isUp(): 指定時間の経過有無（タイマ更新あり）
//  getFreq(): タイマ周期の参照[Hz]
//  getDelta(): タイマ経過の参照[msec]
//  touch(): タイマ更新（ウォッチドッグタイマ等で利用）
//  isOld(): 指定時間の経過有無（タイマ更新なし）
////////////////////////////////////////////////////////////////////////////////
class TimerMS {
  unsigned long last;
  int freq;
public:
  TimerMS() {
    last = 0;
    freq = 1;
  }

  bool isUp(int msec) {
    unsigned long now = millis();
    if (last + msec <= now) {
      freq = 1000 / (now - last); 
      last = now;
      return true;
    }
    return false;
  }
  int getFreq(void) {
    return freq;
  }
  int getDelta(void) {
    return millis() - last; 
  }
  void touch(void) {
    last = millis();
  }
  bool isOld(int msec) {
    unsigned long now = millis();
    return last + msec <= now;
  }
};

class CountHZ {
  unsigned long last;
  int freq, loop;
public:
  CountHZ() {
    last = 0;
    freq = 0;
    loop = 0;
  }
  void touch(void) {
    unsigned long now = millis();
    loop++;
    if (last + 1000 <= now) {
      last = now;
      freq = loop;
      loop = 0;
    }
  }
  int getFreq(void) {
    unsigned long now = millis();
    return last + 1100 < now? 0: freq;
  }
};




////////////////////////////////////////////////////////////////////////////////
// class FilterLP{}: 簡易ローパスフィルタ
//  update(): データ更新
////////////////////////////////////////////////////////////////////////////////
class FilterLP {
  float alpha, yp;
public:
  FilterLP() { setup(); }
  void setup(float _alpha = 0.5, float _yp = 0.0) {
    alpha = _alpha;
    yp = _yp;
  }
  float update(float x) {
    float y = alpha*x + (1.0-alpha)*yp;
    yp = y;
    return y;
  }
};




////////////////////////////////////////////////////////////////////////////////
// class FilterHP{}: 簡易ハイパスフィルタ
//  update(): データ更新
////////////////////////////////////////////////////////////////////////////////
class FilterHP {
  float alpha, yp, xp;
public:
  FilterHP() { setup(); }
  void setup(float _alpha = 0.5, float _yp = 0.0) {
    alpha = _alpha;
    yp = _yp;
    xp = _yp;
  }
  float update(float x) {
    float y = alpha*(x - xp) + alpha*yp;
    yp = y;
    xp = x;
    return y;
  }
};




////////////////////////////////////////////////////////////////////////////////
// class M5StackAHRS{}: 姿勢推定用ライブラリ（可変更新周期、座標変換などに対応）
//  setup(): AHRSの初期化
//  loop(): AHRSの更新
//  initAXIS(): 座標軸の変更（シャーシ固定系の変更）
//  initMEAN(): バイアスの更新（センサのキャリブレーション）
////////////////////////////////////////////////////////////////////////////////
class M5StackAHRS {
  /* AHRS */
  //#include <utility/MahonyAHRS.h>
  //---------------------------------------------------------------------------------------------------
  // Definitions
  
  //#define sampleFreq  25.0f     // sample frequency in Hz
  float sampleFreq = 25.0;
  #define twoKpDef  (2.0f * 1.0f) // 2 * proportional gain
  #define twoKiDef  (2.0f * 0.0f) // 2 * integral gain
  
  //#define twoKiDef  (0.0f * 0.0f)
  
  //---------------------------------------------------------------------------------------------------
  // Variable definitions
  
  volatile float twoKp = twoKpDef;                      // 2 * proportional gain (Kp)
  volatile float twoKi = twoKiDef;                      // 2 * integral gain (Ki)
  volatile float q0 = 1.0, q1 = 0.0, q2 = 0.0, q3 = 0.0;          // quaternion of sensor frame relative to auxiliary frame
  volatile float integralFBx = 0.0f,  integralFBy = 0.0f, integralFBz = 0.0f; // integral error terms scaled by Ki

  //---------------------------------------------------------------------------------------------------
  // IMU algorithm update
  
  void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay, float az,float *pitch,float *roll,float *yaw) {
    float recipNorm;
    float halfvx, halfvy, halfvz;
    float halfex, halfey, halfez;
    float qa, qb, qc;
  
  
    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
  
      // Normalise accelerometer measurement
      recipNorm = invSqrt(ax * ax + ay * ay + az * az);
      ax *= recipNorm;
      ay *= recipNorm;
      az *= recipNorm;
  
      // Estimated direction of gravity and vector perpendicular to magnetic flux
      halfvx = q1 * q3 - q0 * q2;
      halfvy = q0 * q1 + q2 * q3;
      halfvz = q0 * q0 - 0.5f + q3 * q3;
  
      
  
      // Error is sum of cross product between estimated and measured direction of gravity
      halfex = (ay * halfvz - az * halfvy);
      halfey = (az * halfvx - ax * halfvz);
      halfez = (ax * halfvy - ay * halfvx);
  
      // Compute and apply integral feedback if enabled
      if(twoKi > 0.0f) {
        integralFBx += twoKi * halfex * (1.0f / sampleFreq);  // integral error scaled by Ki
        integralFBy += twoKi * halfey * (1.0f / sampleFreq);
        integralFBz += twoKi * halfez * (1.0f / sampleFreq);
        gx += integralFBx;  // apply integral feedback
        gy += integralFBy;
        gz += integralFBz;
      }
      else {
        integralFBx = 0.0f; // prevent integral windup
        integralFBy = 0.0f;
        integralFBz = 0.0f;
      }
  
      // Apply proportional feedback
      gx += twoKp * halfex;
      gy += twoKp * halfey;
      gz += twoKp * halfez;
    }
  
    // Integrate rate of change of quaternion
    gx *= (0.5f * (1.0f / sampleFreq));   // pre-multiply common factors
    gy *= (0.5f * (1.0f / sampleFreq));
    gz *= (0.5f * (1.0f / sampleFreq));
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);
  
    // Normalise quaternion
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
  
  
    *pitch = asin(-2 * q1 * q3 + 2 * q0* q2); // pitch
    *roll  = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1); // roll
    *yaw   = atan2(2*(q1*q2 + q0*q3),q0*q0+q1*q1-q2*q2-q3*q3);  //yaw
  
    *pitch *= RAD_TO_DEG;
      *yaw   *= RAD_TO_DEG;
      // Declination of SparkFun Electronics (40°05'26.6"N 105°11'05.9"W) is
      //  8° 30' E  ± 0° 21' (or 8.5°) on 2016-07-19
      // - http://www.ngdc.noaa.gov/geomag-web/#declination
      *yaw   -= 8.5;
      *roll  *= RAD_TO_DEG;
  
    ///Serial.printf("%f    %f    %f \r\n",  pitch, roll, yaw);
  }
  
  //---------------------------------------------------------------------------------------------------
  // Fast inverse square-root
  // See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
  
  float invSqrt(float x) {
    float halfx = 0.5f * x;
    float y = x;
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
    long i = *(long*)&y;
    i = 0x5f3759df - (i>>1);
    y = *(float*)&i;
  #pragma GCC diagnostic warning "-Wstrict-aliasing"
    y = y * (1.5f - (halfx * y * y));
    return y;
  }

  
  /* IMU values */
  float accl[3] = {0.0,0.0,0.0};
  float gyro[3] = {0.0,0.0,0.0};
  /* IMU mean values */
  float ACCL[3] = {0.0,0.0,0.0};
  float GYRO[3] = {0.0,0.0,0.0};
  /* IMU temp */
  float temp = 0.0F;
  /* AHRS */
  float pitch = 0.0F;
  float roll = 0.0F;
  float yaw = 0.0F;
  
  /* Time */
  uint32_t Now = 0;
  uint32_t lastUpdate = 0;
  float deltat = 1.0f;
  
  /* Axis = Body Fixed Frame */
  float X[3] = {1.0,0.0,0.0};
  float Y[3] = {0.0,1.0,0.0};
  float Z[3] = {0.0,0.0,1.0};

  /* LPF */
  FilterLP LPF[6];
  
  /* Vetor Operations */
  float dot(float* a,float* b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
  float norm(float* a) { return sqrt(dot(a,a)); }
  void mul(float* a,float k,float* b) {
    b[0] = k*a[0];
    b[1] = k*a[1];
    b[2] = k*a[2];
  }
  void add(float* a,float* b,float* c) {
    c[0] = a[0] + b[0];
    c[1] = a[1] + b[1];
    c[2] = a[2] + b[2];
  }
  void sub(float* a,float* b,float* c) {
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
  }
  void dup(float* a,float* b) {
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
  }
  void normalize(float* a) { 
    float na = norm(a);
    a[0] /= na;
    a[1] /= na;
    a[2] /= na;
  }
  void cross(float* a,float* b, float* c) {
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];
  }

public:
  /* initialize */
  void initMEAN(int msec = 2000) {
    for (int i=0; i<3; i++) {
      ACCL[i] = 0.0F;
      GYRO[i] = 0.0F;
    }
    
    int N = 0;
    unsigned long int timeout = millis() + msec;
    while (millis() < timeout) {
      M5.IMU.getGyroData(&gyro[0],&gyro[1],&gyro[2]);
      M5.IMU.getAccelData(&accl[0],&accl[1],&accl[2]);
      for (int i=0; i<3; i++) {
        ACCL[i] += accl[i];
        GYRO[i] += gyro[i];
      }
      N++;
      delay(1);
    }
  
    for (int i=0; i<3; i++) {
      ACCL[i] /= N;
      GYRO[i] /= N;
    }
  }
  
  void initAXIS(int xdir = 1) {
    // initial X0
    int xabs = abs(xdir);
    if (xabs>=1 && xabs<=3) {
      X[0] = (xabs==1? xdir/xabs: 0);
      X[1] = (xabs==2? xdir/xabs: 0);
      X[2] = (xabs==3? xdir/xabs: 0);
    }
    
    // Z := Anti Gravity
    dup(ACCL,Z);
    normalize(Z);
  
    // X := X0 - (Z,X0)Z
    float zx = dot(Z,X);
    float zxZ[3];
    mul(Z,zx,zxZ);
    sub(X,zxZ,X);
    normalize(X);
  
    // Y := Z x X
    cross(Z,X,Y);
    normalize(Y);
  }
  
  void setup(int msec = 2000, int xdir = 1) {
    M5.IMU.Init();
    initMEAN(msec);
    initAXIS(xdir);
  }
  
  void loop(float *gyro_=NULL, float *accl_=NULL, float *ahrs_=NULL, float *temp_=NULL) {
    // put your main code here, to run repeatedly:
    M5.IMU.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
    M5.IMU.getAccelData(&accl[0], &accl[1], &accl[2]);
    M5.IMU.getTempData(&temp);

    // remove bias
    for (int i=0; i<3; i++) gyro[i] -= GYRO[i];
    
    // time update
    Now = millis();
    deltat = ((Now - lastUpdate) / 1000.0);
    lastUpdate = Now;
    sampleFreq = 1.0/deltat;

    //M5.IMU.getAhrsData(&pitch,&roll,&yaw);
    MahonyAHRSupdateIMU(dot(gyro,X)*DEG_TO_RAD,dot(gyro,Y)*DEG_TO_RAD,dot(gyro,Z)*DEG_TO_RAD, dot(accl,X),dot(accl,Y),dot(accl,Z), &pitch,&roll,&yaw);

    // copy results
    if (gyro_) {
      gyro_[0] = dot(gyro,X);
      gyro_[1] = dot(gyro,Y);
      gyro_[2] = dot(gyro,Z);
    }
    if (accl_) {
      accl_[0] = dot(accl,X);
      accl_[1] = dot(accl,Y);
      accl_[2] = dot(accl,Z);
    }
    if (ahrs_) {
      ahrs_[0] = roll;
      ahrs_[1] = pitch;
      ahrs_[2] = yaw;
    }
    if (temp_) {
      *temp_ = temp;
    }
  }
  void debug() {
    DEBUG.println("M5StackAHRS:");
    DEBUG.printf(" gyro=(%.2f,%.2f,%.2f)\n",gyro[0],gyro[1],gyro[2]);
    DEBUG.printf(" accl=(%.2f,%.2f,%.2f)\n",accl[0],accl[1],accl[2]);
    DEBUG.printf(" ahrs=(%.2f,%.2f,%.2f)\n",roll,pitch,yaw);
    DEBUG.printf(" GYRO=(%.2f,%.2f,%.2f)\n",GYRO[0],GYRO[1],GYRO[2]);
    DEBUG.printf(" ACCL=(%.2f,%.2f,%.2f)\n",ACCL[0],ACCL[1],ACCL[2]);
    DEBUG.printf(" X=(%.2f,%.2f,%.2f)\n",X[0],X[1],X[2]);
    DEBUG.printf(" Y=(%.2f,%.2f,%.2f)\n",Y[0],Y[1],Y[2]);
    DEBUG.printf(" Z=(%.2f,%.2f,%.2f)\n",Z[0],Z[1],Z[2]);   
  }
  
  int getFreq(void) { return int(1.0/deltat); }
  float getAccT(void) { return LPF[0].update(dot(accl,X)); }
  float getAccL(void) { return LPF[1].update(dot(accl,Y)); }
  float getAccV(void) { return dot(accl,Z); }
  float getYawRate(void) { return LPF[2].update(dot(gyro,Z)); }
  float getRoll(void) {
    float Roll = - atan2(dot(accl,X),dot(accl,Z));
    return LPF[3].update(Roll * (180.0/PI));
  }
  float getPitch(void) {
    float Pitch = atan2(dot(accl,Y),dot(accl,Z));
    return LPF[4].update(Pitch * (180.0/PI));
  }
  float getTraction(float G0, float y0=0.1) {
    float x = getAccT()/G0;
    return fabs(x)>=1.0? y0: sqrt(1.-x*x); 
  }

};




#endif
