////////////////////////////////////////////////////////////////////////////////
// DIY-RCシステムのTX専用ソース
// DIY radio control system by M5Stack series
// https://github.com/hshin-git/M5RadioControl
////////////////////////////////////////////////////////////////////////////////
#ifndef _M5RC_TX_H_
#define _M5RC_TX_H_


//#define DEBUG M5.Lcd
#define DEBUG Serial


////////////////////////////////////////////////////////////////////////////////
// class M5_ADS1X15{}: 送信機用のADコンバータ(I2C接続ADC、4ch中の1chは基準電圧計測に使うので正味3ch）
//  setup(): 初期化
//  getCount(): 電圧[カウント]
//  getVolts(): 電圧[V]
//  getPercent(): 電圧比率[%]
//  getPercents(): 電圧比率[%]
////////////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

class M5_ADS1X15 {
  Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
  //Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

public:
  void setup(int addr=0x48, int sda=32,int sck=33) {
    Wire.begin(sda,sck,800000L);
    while (1) {
      if (ads.begin(addr,&Wire)) {
        DEBUG.printf("Found ADS1X15 at (addr=0x%x,sda=%d,sck=%d).\n",addr,sda,sck);
        break;
      }
      else {
        DEBUG.println("Failed to initialize ADS.");
        delay(1000);
      }
    }

    //ads.setDataRate(RATE_ADS1115_128SPS);
    //ads.setDataRate(RATE_ADS1115_250SPS);
    //ads.setDataRate(RATE_ADS1115_475SPS);
    ads.setDataRate(RATE_ADS1115_860SPS);
    
    DEBUG.println("Getting single-ended readings from AIN0..3");
    DEBUG.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
  
    // The ADC input range (or gain) can be changed via the following
    // functions, but be careful never to exceed VDD +0.3V max, or to
    // exceed the upper and lower limits if you adjust the input range!
    // Setting these values incorrectly may destroy your ADC!
    //                                                                ADS1015  ADS1115
    //                                                                -------  -------
    // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  }

  int getCounts(int ch) {
    if (ch>=0 && ch<4) {
      return ads.readADC_SingleEnded(ch);
    }
    return -1;
  }
  float getVolts(int ch) {
    if (ch>=0 && ch<4) {
      int cnt = ads.readADC_SingleEnded(ch);
      return ads.computeVolts(cnt);
    }
    return -1;
  }
  float getPercent(int ch) {
    if (ch>=0 && ch<3) {
      int c0 = ads.readADC_SingleEnded(ch);
      int c3 = ads.readADC_SingleEnded(3);
      return c3>0? (c0*100.0)/c3: -1;
    }
    return -1;
  }
  bool getPercents(float* ps, int nc) {
    if (ps && nc>0) {
      int c3 = ads.readADC_SingleEnded(3);
      for (int ch=0; ch<nc; ch++) {
        int c0 = ads.readADC_SingleEnded(ch);
        ps[ch] = c3>0? (c0*100.0)/c3: -1;
      }
      return true;
    }
    return false;
  }
  
};




////////////////////////////////////////////////////////////////////////////////
// class SDCserver{}: SDカードのWiFi/HTTPアクセス機能（WiFiをAPモードで起動、SDカード内容をHTTPサーバ公開）
//  setup(): サーバ起動
//  loop(): サーバ実行
//  stop(): サーバ停止
// https://github.com/esp8266/ESPWebServer/tree/master/examples/SDWebServer
////////////////////////////////////////////////////////////////////////////////
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>


static const char* _SSID = "M5TX";
static WebServer _SERVER(80);
static bool hasSD = false;
static File uploadFile;
static bool firstTime = true;
  
class SDCServer {
  static void returnOK() {
    _SERVER.send(200, "text/plain", "");
  }
  
  static void returnFail(String msg) {
    _SERVER.send(500, "text/plain", msg + "\r\n");
  }
  
  static void listDirectory(String path) {
    if (!SD.exists((char *)path.c_str())) {
      return returnFail("BAD PATH");
    }
    File dir = SD.open((char *)path.c_str());
    //path = String();
    if (!dir.isDirectory()) {
      dir.close();
      return returnFail("NOT DIR");
    }
    dir.rewindDirectory();
    _SERVER.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _SERVER.send(200, "text/html", "");
    WiFiClient client = _SERVER.client();
  
    //_SERVER.sendContent("[");
    _SERVER.sendContent("<!DOCTYPE html><html><body>");
    _SERVER.sendContent("Index of " + path);
    _SERVER.sendContent("<ul>");
    for (int cnt = 0; true; ++cnt) {
      File entry = dir.openNextFile();
      if (!entry) {
        break;
      }
      
      String output;
  #if 0
      if (cnt > 0) {
        output = ',';
      }
      output += "{\"type\":\"";
      output += (entry.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += entry.name();
      output += "\"";
      output += "}";
  #else
      output += "<li><a href=\"";
      output += (entry.isDirectory()? entry.name(): path+"/"+entry.name());
      output += "\">";
      output += entry.name();
      output += "</a>";
  #endif
      _SERVER.sendContent(output);
      entry.close();
    }
    //_SERVER.sendContent("]");
    _SERVER.sendContent("</ul></body></html>");
    dir.close();
  }
  
  static bool loadFromSdCard(String path) {
    String dataType = "text/plain";
    //if (path.endsWith("/")) {
    //  path += "index.htm";
    //}
  
    if (path.endsWith(".src")) {
      path = path.substring(0, path.lastIndexOf("."));
    } else if (path.endsWith(".htm")) {
      dataType = "text/html";
    } else if (path.endsWith(".css")) {
      dataType = "text/css";
    } else if (path.endsWith(".js")) {
      dataType = "application/javascript";
    } else if (path.endsWith(".png")) {
      dataType = "image/png";
    } else if (path.endsWith(".gif")) {
      dataType = "image/gif";
    } else if (path.endsWith(".jpg")) {
      dataType = "image/jpeg";
    } else if (path.endsWith(".ico")) {
      dataType = "image/x-icon";
    } else if (path.endsWith(".xml")) {
      dataType = "text/xml";
    } else if (path.endsWith(".pdf")) {
      dataType = "application/pdf";
    } else if (path.endsWith(".zip")) {
      dataType = "application/zip";
    } else if (path.endsWith(".csv")) {
      dataType = "text/csv";
    } else if (path.endsWith(".txt")) {
      dataType = "text/txt";
    }
    
    File dataFile = SD.open(path.c_str());
    if (!dataFile) {
      return false;
    }
    
    if (dataFile.isDirectory()) {
      if (SD.exists((char *)(path + "/index.htm").c_str())) {
        path += "/index.htm";
        dataType = "text/html";
        dataFile = SD.open(path.c_str());
      } else {
        dataFile.close();
        listDirectory(path);
        return true;
      }
    }
  
    if (_SERVER.hasArg("download")) {
      dataType = "application/octet-stream";
    }
  
    if (_SERVER.streamFile(dataFile, dataType) != dataFile.size()) {
      DEBUG.println("Sent less data than expected!");
    }
  
    dataFile.close();
    return true;
  }
  
  static void handleFileUpload() {
    if (_SERVER.uri() != "/edit") {
      return;
    }
    HTTPUpload& upload = _SERVER.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (SD.exists((char *)upload.filename.c_str())) {
        SD.remove((char *)upload.filename.c_str());
      }
      uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
      DEBUG.print("Upload: START, filename: "); DEBUG.println(upload.filename);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) {
        uploadFile.write(upload.buf, upload.currentSize);
      }
      DEBUG.print("Upload: WRITE, Bytes: "); DEBUG.println(upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
      }
      DEBUG.print("Upload: END, Size: "); DEBUG.println(upload.totalSize);
    }
  }
  
  static void deleteRecursive(String path) {
    File file = SD.open((char *)path.c_str());
    if (!file.isDirectory()) {
      file.close();
      SD.remove((char *)path.c_str());
      return;
    }
  
    file.rewindDirectory();
    while (true) {
      File entry = file.openNextFile();
      if (!entry) {
        break;
      }
      String entryPath = path + "/" + entry.name();
      if (entry.isDirectory()) {
        entry.close();
        deleteRecursive(entryPath);
      } else {
        entry.close();
        SD.remove((char *)entryPath.c_str());
      }
      yield();
    }
  
    SD.rmdir((char *)path.c_str());
    file.close();
  }
  
  static void handleDelete() {
    if (_SERVER.args() == 0) {
      return returnFail("BAD ARGS");
    }
    String path = _SERVER.arg(0);
    if (path == "/" || !SD.exists((char *)path.c_str())) {
      returnFail("BAD PATH");
      return;
    }
    deleteRecursive(path);
    returnOK();
  }
  
  static void handleCreate() {
    if (_SERVER.args() == 0) {
      return returnFail("BAD ARGS");
    }
    String path = _SERVER.arg(0);
    if (path == "/" || SD.exists((char *)path.c_str())) {
      returnFail("BAD PATH");
      return;
    }
  
    if (path.indexOf('.') > 0) {
      File file = SD.open((char *)path.c_str(), FILE_WRITE);
      if (file) {
        file.write(0);
        file.close();
      }
    } else {
      SD.mkdir((char *)path.c_str());
    }
    returnOK();
  }
  
  static void printDirectory() {
    if (!_SERVER.hasArg("dir")) {
      return returnFail("BAD ARGS");
    }
    String path = _SERVER.arg("dir");
    if (path != "/" && !SD.exists((char *)path.c_str())) {
      return returnFail("BAD PATH");
    }
    File dir = SD.open((char *)path.c_str());
    path = String();
    if (!dir.isDirectory()) {
      dir.close();
      return returnFail("NOT DIR");
    }
    dir.rewindDirectory();
    _SERVER.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _SERVER.send(200, "text/json", "");
    WiFiClient client = _SERVER.client();
  
    _SERVER.sendContent("[");
    for (int cnt = 0; true; ++cnt) {
      File entry = dir.openNextFile();
      if (!entry) {
        break;
      }
  
      String output;
      if (cnt > 0) {
        output = ',';
      }
  
      output += "{\"type\":\"";
      output += (entry.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += entry.name();
      output += "\"";
      output += "}";
      _SERVER.sendContent(output);
      entry.close();
    }
    _SERVER.sendContent("]");
    dir.close();
  }
  
  static void handleNotFound() {
    if (hasSD && loadFromSdCard(_SERVER.uri())) {
      return;
    }
    String message = "SDCARD Not Detected\n\n";
    message += "URI: ";
    message += _SERVER.uri();
    message += "\nMethod: ";
    message += (_SERVER.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += _SERVER.args();
    message += "\n";
    for (uint8_t i = 0; i < _SERVER.args(); i++) {
      message += " NAME:" + _SERVER.argName(i) + "\n VALUE:" + _SERVER.arg(i) + "\n";
    }
    _SERVER.send(404, "text/plain", message);
    DEBUG.print(message);
  }

public:
  static void setup(void) {
  
    DEBUG.println("Setup WIFI AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_SSID);
    WiFi.begin();
    delay(500);
    
    DEBUG.print("Started! IP address: ");
    DEBUG.println(WiFi.softAPIP());
  
    if (MDNS.begin(_SSID)) {
      MDNS.addService("http", "tcp", 80);
      DEBUG.println("MDNS responder started");
      DEBUG.print("You can now connect to http://");
      DEBUG.print(_SSID);
      DEBUG.println(".local");
    }
  
    if (!firstTime) return;
    _SERVER.on("/list", HTTP_GET, printDirectory);
    _SERVER.on("/edit", HTTP_DELETE, handleDelete);
    _SERVER.on("/edit", HTTP_PUT, handleCreate);
    _SERVER.on("/edit", HTTP_POST, []() { returnOK(); }, handleFileUpload);
    _SERVER.onNotFound(handleNotFound);
    _SERVER.begin();
    DEBUG.println("HTTP _SERVER started");
    if (SD.begin()) {
      DEBUG.println("SD Card initialized.");
      hasSD = true;
    }
    firstTime = false;
  }
  
  static void loop(void) {
    _SERVER.handleClient();
  }
  
  static void stop(void) {
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect();
  }

  IPAddress getIP(void) {
    return WiFi.softAPIP();
  }

};


////////////////////////////////////////////////////////////////////////////////
// class DataFIFO: マルチスレッド対応リングバッファ（別スレッドで追加と取得OK）
// put(): データの追加
// get(): データの取得
// gets(): データの取得（複数アイテム対応） 
////////////////////////////////////////////////////////////////////////////////
template <class DATA> class DataFIFO {
  DATA *RING;
  int SIZE;
  int head;
  int tail;
  // FreeRTOS
  SemaphoreHandle_t xMutex;
  
public:
  DataFIFO(int N = 32, bool mutex = false) {
    RING = new DATA[N];
    SIZE = N;
    head = tail = 0;
    // FreeRTOS
    xMutex = mutex? xSemaphoreCreateMutex(): NULL;
  }

  bool empty(void) { return head == tail; }
  bool full(void) { return head == (tail + 1) % SIZE; }
  int count(void) { return (tail - head + SIZE) % SIZE; }

  void lock(void) { if (xMutex) xSemaphoreTake(xMutex,portMAX_DELAY); }
  void unlock(void) { if (xMutex) xSemaphoreGive(xMutex); }

  bool put(DATA *d) {
    bool code = false;
    lock();
    if (full()) {
      code = false;
    } 
    else {
      RING[tail] = *d;
      tail = (tail + 1) % SIZE;
      code = true;
    }
    unlock();
    return code;
  }
  
  bool get(DATA *d) {
    bool code = false;
    lock();
    if (empty()) {
      code = false;
    }
    else {
      *d = RING[head];
      head = (head + 1) % SIZE;
      code = true;
    }
    unlock();
    return code;
  }

  bool gets(DATA **pData, int *pSize) {
    bool code = false;
    lock();
    if (empty()) {
      code = false;
    }
    else {
      if (head < tail) {
        *pData = &RING[head];
        *pSize = (tail - head);
        head = tail;
      }
      else {
        *pData = &RING[head];
        *pSize = (SIZE - head);
        head = 0;
      }
      code = true;
    }
    unlock();
    return code;
  }

  int gets(DATA *buf, int len) {
    int code = -1;
    lock();
    if (empty()) {
      code = 0;
    }
    else {
      int N = min(len, count());
      DATA* src = &RING[head];
      DATA* dst = &buf[0];
    
      if (head < tail) {
        memcpy(dst,src,N*sizeof(DATA));
        head += N;
      }
      else {
        int n = min(N,SIZE-head);
        memcpy(dst,src,n*sizeof(DATA));
        if (n < N) {
          src = &RING[0];
          dst = &buf[n];
          memcpy(dst,src,(N-n)*sizeof(DATA));
        }
        head = (head + N) % SIZE;
      }
      code = N;
    }
    unlock();
    return code;
  }
  
};


////////////////////////////////////////////////////////////////////////////////
// class DataFIFO: ログファイルの管理（SDカードへの遅延書き込み対応）
// open(): ログファイルの開始
// write(): ログデータの追加（受信スレッドで呼び出す）
// flush(): ログデータの保存
// close(): ログバイルの終了
////////////////////////////////////////////////////////////////////////////////
template <class DATA> class DataFILE {
  static const int NCOL = sizeof(DATA)/sizeof(float);
  DataFIFO<DATA>* FIFO;
  DATA* BUFF;
  int SIZE;
  File FILE;
  //const char** COLS;
  // monitoring
  bool OPENED;
  unsigned int START, COUNT;
    
public:
  //DataFILE(const char** NAME, int NBUF = 16) {
  DataFILE(int NBUF = 16) {
    FIFO = new DataFIFO<DATA>(NBUF,true);
    BUFF = new DATA[NBUF];
    SIZE = NBUF;
    //COLS = NAME;
    //
    OPENED = false;
    START = 0;
    COUNT = 0;
  }
  
  bool open(const char *filepath) {
    FILE = SD.open(filepath, FILE_WRITE);
    if (FILE) {
      START = millis();
      COUNT = 0;
      OPENED = true;
      return true;
    }
    return false;
  }
  void close(void) {
    OPENED = false;
    if (FILE) {
      DATA d;
      while (FIFO->get(&d)) {
        float *v = (float*)&d;
        FILE.write((const uint8_t*)v, (size_t)sizeof(DATA));
      }
      FILE.close();
    }
  }
    
  bool write(DATA *d) {
    if (OPENED && FILE && !FIFO->full()) {
      if (COUNT == 0) {
        START = d->msec;
        d->msec = 0;
      } else {
        d->msec -= START;
      }
      COUNT++;
      FIFO->put(d);
      return true;
    }
    return false;
  }
  bool flush(int level = 0) {
    if (OPENED && FILE && (FIFO->count() > level)) {
      //unsigned long enter = micros();
      int n;
      while ((n = FIFO->gets(BUFF,SIZE)) > 0) {
        FILE.write((const uint8_t*)BUFF, (size_t)(n*sizeof(DATA)));
      }
      //Serial.printf("flush: delta = %d usec\n", micros() - enter);
      return true;
    }
    return false;
  }


  // for monitoring
  bool isOpen(void) { return OPENED; }
  int getCount(void) { return COUNT; }
  int getDelta(void) { return millis() - START; }
  int getFreq(void) { return 1000 * COUNT / getDelta(); }

#if 0
  // for convert bin to csv
  int toCSV(const char* binPath, const char* csvPath) {
    if (FILE) return -1;
    File bin = SD.open(binPath, FILE_READ);
    File csv = SD.open(csvPath, FILE_WRITE);
    int count = 0;
    if (bin && csv) {
      // header
      for (int i=0; i<NCOL; i++) csv.printf("%s%s", COLS[i], i<NCOL-1? ",": "\n");
      // content
      size_t bsz = sizeof(DATA);
      float dat[NCOL];
      while (bin.read((uint8_t*)dat,bsz) == bsz) {
        for (int i=0; i<NCOL; i++) csv.printf("%.3f%s", dat[i], i<NCOL-1? ",": "\n");
        count++;
      }
    }
    return count;
  }
  
  // for ploting
  int compStats(const char* binPath,float MIN[NCOL], float MEAN[NCOL], float MAX[NCOL], float STD[NCOL], float CC[NCOL][NCOL]) {
    if (FILE) return -1;
    File file = SD.open(binPath,FILE_READ);
    if (file == NULL) return -2;
    
    float SUM[NCOL], VAR[NCOL][NCOL];
    
    // (1) init
    for (int i=0; i<NCOL; i++) {
      if (MIN!=NULL) MIN[i] = 1e+9;
      if (MAX!=NULL) MAX[i] = -1e+9;
      SUM[i] = 0.0F;
      for (int j=0; j<NCOL; j++) VAR[i][j] = 0.0F;
    }
    
    // (2) scan
    float DAT[NCOL];
    size_t bsz = sizeof(DATA);
    int NUM = 0;
    while (file.read((uint8_t*)DAT, bsz) == bsz) {
      for (int i=0; i<NCOL; i++) {
        if (MIN && DAT[i]<MIN[i]) MIN[i] = DAT[i];
        if (MAX && DAT[i]>MAX[i]) MAX[i] = DAT[i];
        SUM[i] += DAT[i];
        for (int j=0; j<NCOL; j++) {
          VAR[i][j] += DAT[i]*DAT[j];
        }
      }
      NUM++;
    }
    
    // (3) stat
    for (int i=0; i<NCOL; i++) {
      if (MEAN) MEAN[i] = (SUM[i]/NUM);
      if (STD) STD[i] = sqrt( (VAR[i][i]/NUM) - (SUM[i]/NUM)*(SUM[i]/NUM) );
    }
    for (int i=0; i<NCOL; i++) {
      for (int j=0; j<NCOL; j++) {
        if (CC && STD) CC[i][j] = STD[i]*STD[j]? (VAR[i][j]/NUM) / (STD[i]*STD[j]): 0.0; 
      }
    }
    return NUM;
  }
#endif

};





#endif
