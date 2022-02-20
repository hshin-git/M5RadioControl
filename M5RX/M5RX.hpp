////////////////////////////////////////////////////////////////////////////////
// DIY-RCシステムのRX専用ソース
// DIY radio control system by M5Stack series
// https://github.com/hshin-git/M5RadioControl
////////////////////////////////////////////////////////////////////////////////
#ifndef _M5RC_RX_H_
#define _M5RC_RX_H_


//#define DEBUG M5.Lcd
#define DEBUG Serial


////////////////////////////////////////////////////////////////////////////////
// class PulsePort{}: PWM信号の入出力ライブラリ
//  setup(): 入力ピンの初期化
//  getUsec(): 入力パルス幅[usec]
//  getFreq(): 入力パルス周波数[Hz]
//  attach(): 割り込み処理の再開
//  detach(): 割り込み処理の中止
//  setupOut(): 出力ピンの初期化
//  putUsec(): 出力パルス幅[usec]
//  putFreq(): 出力パルス周波数[Hz]
////////////////////////////////////////////////////////////////////////////////
// PWM watch dog timer
#include <Ticker.h>

// PWM pulse in
typedef struct {
  int pin;
  int tout;
  // for pulse
  int dstUsec;
  int prev;
  unsigned long last;
  // for freq
  int dstFreq;
  unsigned long lastFreq;
} InPulse;

// PWM pulse out
typedef struct {
  int pin;
  int freq;
  int bits;
  int duty;
  int usec;
  int dstUsec;
} OutPulse;

class PulsePort {
  static const int MAX = 4; // max of channels 

  static int InCH;   // number of in-channels
  static int OutCH;   // number of out-channels
  
  static InPulse IN[MAX]; // pwm in-pulse
  static OutPulse OUT[MAX]; // pwm out-pulse

  static Ticker WDT;      // watch dog timer
  static bool WATCHING;
  static float MEAN[MAX]; // mean of pwm in-pulse
  
  static void ISR(void *arg) {
    unsigned long tnow = micros();
    int ch = (int)arg;
    InPulse* pwm = &IN[ch];
    int vnow = digitalRead(pwm->pin);
    
    if (pwm->prev==0 && vnow==1) {
      // at up edge
      pwm->prev = 1;
      pwm->last = tnow;
      // for freq
      pwm->dstFreq = (tnow > pwm->lastFreq? 1000000/(tnow - pwm->lastFreq): 0);
      pwm->lastFreq = tnow;
    }
    else
    if (pwm->prev==1 && vnow==0) {
      // at down edge
      pwm->dstUsec = tnow - pwm->last;
      pwm->prev = 0;
      pwm->last = tnow;
    }
  }

  static void TSR(void) {
    unsigned long tnow = micros();
    for (int ch=0; ch<InCH; ch++) {
      InPulse* pwm = &IN[ch];
      if (pwm->last + pwm->tout < tnow) {
        pwm->dstUsec = 0;
        pwm->dstFreq = 0;
      }
    }
  }

public: 
  PulsePort() {
    // do nothing
  };
  static int setup(int pin, int toutUs=21*1000) {
    int ch = -1;
    if (InCH < MAX) {
      ch = InCH++;
      InPulse* pwm = &IN[ch];
      //
      pwm->pin = pin;
      pwm->tout = toutUs;
      // for pulse
      pwm->dstUsec = 0;
      pwm->prev = 0;
      pwm->last = micros();
      // for freq
      pwm->dstFreq = 0;
      pwm->lastFreq = micros();
      //
      pinMode(pin,INPUT);
      attachInterruptArg(pin,&ISR,(void*)ch,CHANGE);
      if (ch == 0) WDT.attach_ms(pwm->tout/1000,&TSR);
      WATCHING = true;
    }
    return ch;
  };
  static int getUsec(int ch) {
    if (ch >= 0 && ch < InCH) {
      InPulse* pwm = &IN[ch];
      return pwm->dstUsec;
    }
    return -1;
  }
  static int getFreq(int ch) {
    if (ch >= 0 && ch < InCH) {
      InPulse* pwm = &IN[ch];
      return pwm->dstFreq;
    }
    return -1;
  }
  
  static void detach(void) {
    if (InCH == 0) return;
    WDT.detach();
    for (int ch=0; ch<InCH; ch++) {
      InPulse *pwm = &IN[ch];
      detachInterrupt(pwm->pin);
    }
    WATCHING = false;
  };
  static void attach(void) {
    if (InCH == 0 || WATCHING) return;
    for (int ch=0; ch<InCH; ch++) {
      InPulse *pwm = &IN[ch];
      attachInterruptArg(pwm->pin,&ISR,(void*)ch,CHANGE);
      if (ch == 0) WDT.attach_ms(pwm->tout/1000,&TSR);
    }
    WATCHING = true;
  };

  // ESP32's PWM channel 0 and 1 have common frequency.
  static inline int CH2PWM(int ch) { return (ch)*2; };

  static int setupOut(int pin, int freq = 50, int bits = 16) {
    int ch = -1;
    if (OutCH < MAX) {
      ch = OutCH++;
      OutPulse* out = &OUT[ch];
      out->pin = pin;
      out->freq = freq;
      out->bits = bits;
      out->duty = (1 << bits);
      out->usec = 1000000/freq;
      //
      pinMode(out->pin,OUTPUT);
      ledcSetup(CH2PWM(ch),out->freq,out->bits);
      ledcWrite(CH2PWM(ch),0);
      ledcAttachPin(out->pin,CH2PWM(ch));
      //DEBUG.printf("setupOut: ch=%d freq=%d bits=%d usec=%d\n",ch,out->freq,out->bits,out->usec);
    }
    return ch;
  }
  static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
  static bool putUsec(int ch, float usec) {
    if (ch >= 0 && ch < OutCH) {
      OutPulse* out = &OUT[ch];
      int duty = mapFloat(usec, 0,out->usec, 0,out->duty);
      ledcWrite(CH2PWM(ch), duty);
      out->dstUsec = usec;
      return true;
    }
    return false;
  }
  static bool putFreq(int ch, int freq) {
    if (ch >= 0 && ch < OutCH) {
      OutPulse* out = &OUT[ch];
      out->freq = freq;
      out->usec = 1000000/freq;
      //
      ledcWrite(CH2PWM(ch),0);
      ledcDetachPin(out->pin);
      ledcSetup(CH2PWM(ch),out->freq,out->bits);
      ledcWrite(CH2PWM(ch),0);
      ledcAttachPin(out->pin,CH2PWM(ch));
      //DEBUG.printf("putFreq: ch=%d freq=%d bits=%d usec=%d\n",ch,out->freq,out->bits,out->usec);
      return true;
    }
    return false;
  }

  static void setupMean(bool first = false, int msec = 1000) {
    if (first) {
      unsigned long int timeout = millis() + msec;
      int count = 0;
      for (int ch=0; ch<MAX; ch++) MEAN[ch] = 0.0F;
      while (timeout > millis()) {
        for (int ch=0; ch<MAX; ch++) MEAN[ch] += getUsec(ch);
        count++;
        delay(5);
      }
      for (int ch=0; ch<MAX; ch++) MEAN[ch] /= count;
    }
//    detach();
//    if (PREFS.begin(M5LOGGER)) {
//      if (first)
//        PREFS.putBytes("servo", (uint8_t*)MEAN, sizeof(MEAN));
//      else
//        PREFS.getBytes("servo", (uint8_t*)MEAN, sizeof(MEAN));
//      PREFS.end();
//    }
//    attach();
  }
  static float getUsecMean(int ch) { return MEAN[ch]; }

  static void dump(void) {
    for (int ch=0; ch<InCH; ch++) {
      InPulse* pwm = &IN[ch];
      DEBUG.printf(" in(%d): pin=%2d pulse=%6d (usec) freq=%4d (Hz)\n", ch,pwm->pin,pwm->dstUsec,pwm->dstFreq);
    }
    for (int ch=0; ch<OutCH; ch++) {
      OutPulse* out = &OUT[ch];
      DEBUG.printf("out(%d): pin=%2d pulse=%6d (usec) freq=%4d (Hz)\n", ch,out->pin,out->dstUsec,out->freq);
    }
  }

};

// initialization for static class member
int PulsePort::InCH = 0;
int PulsePort::OutCH = 0;

InPulse PulsePort::IN[PulsePort::MAX];
OutPulse PulsePort::OUT[PulsePort::MAX];

Ticker PulsePort::WDT;
bool PulsePort::WATCHING = false;
float PulsePort::MEAN[PulsePort::MAX];



////////////////////////////////////////////////////////////////////////////////
// class M5_DRV8833{}: フルブリッド型ドライバ利用ESC（Arduino版OSSをESP32化、PWM周波数可変機能を追加）
//  setup(): ESCの初期化
//  drive(): ESCの速度調整
// https://github.com/TheDIYGuy999/DRV8833
// DRV8833.h - Library for the Texas Instruments DRV8833 motor driver.
// Created by TheDIYGuy999 June 2016
// Released into the public domain.
////////////////////////////////////////////////////////////////////////////////
class M5_DRV8833 {
  // PWM parameters
  const int PWM_BITS = 8;
  const int MAX_DUTY = (1<<PWM_BITS)-1;
  int _ch1;
  int _ch2;
  // VVVF parameters
  int _freqBase;  // from _freqBase to 2*_freqBase
  int _freqRamp;  // ramp up time in ms
  // VVVF state vars
  int _prevFreq;
  int _prevUsec;
  int _prevStep;
  unsigned long _prevTime = 0;
  // VVVF function
  void _shiftFreq(int Usec) {
    if (_freqRamp <= 0) return;
    const int MODE = 5;
    const int STEP = 10;
    unsigned long Time = millis();
    if (Time >= _prevTime + _freqRamp/STEP) {
      _prevTime = Time;
      /* update _prevStep */
      if (_freqRamp > 100) {
        // ramping frequency by time
        if (Usec >= _maxNeutral) {  // Forward
          // shift up or down
          _prevStep = (Usec >= _prevUsec-10)? _prevStep + 1: _prevStep - 1;
        } else
        if (Usec <= _minNeutral) {  // Backward
          // shift up or down
          _prevStep = (Usec <= _prevUsec+10)? _prevStep + 1: _prevStep - 1;
        } 
        else {  // Neutral
          // reset
          _prevStep = 0;
        }
      } else {
        // ramping frequency by duty
        if (Usec >= _maxNeutral) {  // Forward
          _prevStep = map(Usec-_maxNeutral, 0,500, 0,MODE*STEP);
        } else
        if (Usec <= _minNeutral) {  // Backward
          _prevStep = map(Usec-_minNeutral, 0,-500, 0,MODE*STEP);
        }
        else {  // Neutral
          _prevStep = 0; 
        }
      }
      _prevStep = constrain(_prevStep, 0,MODE*STEP);
      /* update _prevFreq */
      for (int m = 0; m < MODE; m++) {
        int m1 = m + 1;
        if (_prevStep < m1*STEP) {
          _prevFreq = _freqBase + _freqBase * (_prevStep - m*STEP) / (m1*STEP);
          break;
        }
      }
      if (_prevStep >= MODE*STEP) _prevFreq = _freqBase;
      _prevFreq = constrain(_prevFreq, _freqBase,2*_freqBase);
      /* set frequency */
      ledcSetup(_ch1,_prevFreq,PWM_BITS);
      ledcSetup(_ch2,_prevFreq,PWM_BITS);
      //DEBUG.printf("[%02d]:%5dHz\n",_prevStep,_prevFreq);
    }
    _prevUsec = Usec;
  }
private:
  int _pin1;
  int _pin2;
  int _minInput;
  int _maxInput;
  int _minNeutral;
  int _maxNeutral;
  int _controlValue;
  int _controlValueRamp;
  int _rampTime;
  boolean _brake;
  boolean _neutralBrake;
  boolean _invert;
  boolean _doublePWM;
  unsigned long _previousMillis = 0;
  byte _state = 0;
  /* ESP32 */
  inline int CH2PWM(int ch) { return 2*ch; };
  inline void _analogWrite(int chan, int duty) {
    duty = constrain(duty, 0,MAX_DUTY);
#if ESP32
    ledcWrite(chan,duty);
#else
    analogWrite(chan,duty);
#endif
    //DEBUG.printf("chan=%02d duty=%4d\r\n",chan,duty);
  }
public:
  // NOTE: The first pin must always be PWM capable, the second only, if the last parameter is set to "true"
  // SYNTAX: IN1, IN2, min. input value, max. input value, neutral position width
  // invert rotation direction, true = both pins are PWM capable
  M5_DRV8833(int pin1, int pin2, int ch1=0, int ch2=0, int minInput=1000, int maxInput=2000, int neutralWidth=80, boolean invert=false, boolean doublePWM=true) { // Constructor
      _pin1 = pin1;
      _pin2 = pin2;
      _minInput = minInput;
      _maxInput = maxInput;
      _minNeutral = (_maxInput + _minInput) / 2 - (neutralWidth / 2);
      _maxNeutral = (_maxInput + _minInput) / 2 + (neutralWidth / 2);
      _controlValueRamp = (_minNeutral + _maxNeutral) / 2;
      _invert = invert;
      _doublePWM = doublePWM;
      _state = 0;
      _previousMillis = 0;
      //_ch1 = CH2PWM(ch1);
      //_ch2 = CH2PWM(ch2);
      _ch1 = ch1;
      _ch2 = ch2;
#if ESP32
      pinMode(_pin1, OUTPUT);
      pinMode(_pin2, OUTPUT);
      digitalWrite(_pin1, 0);
      digitalWrite(_pin2, 0);
      //_prevFreq = PWM_FREQ;
      //ledcSetup(_ch1,_prevFreq,PWM_BITS);
      //ledcSetup(_ch2,_prevFreq,PWM_BITS);
      setup(false,1024,0);
      ledcWrite(_ch1,0);
      ledcWrite(_ch2,0);
      ledcAttachPin(_pin1,_ch1);
      ledcAttachPin(_pin2,_ch2);
#else
      _ch1 = _pin1;
      _ch2 = _pin2;
      pinMode(_pin1, OUTPUT);
      pinMode(_pin2, OUTPUT);
      digitalWrite(_pin1, LOW);
      digitalWrite(_pin2, LOW);
#endif
  }

  // SYNTAX: Input value, max PWM, ramptime in ms per 1 PWM increment
  // true = brake active, false = brake in neutral position inactive
  void setup(boolean brakeMode=false, int freqBase=1024, int freqRamp=0) {
    // brake mode
    _brake = brakeMode;
    _neutralBrake = brakeMode;
    _rampTime = 0;
    // sound mode
    _freqBase = freqBase;
    _freqRamp = freqRamp;
    // setup PWM
    _prevUsec = 1500;
    _prevFreq = _freqBase;
    _prevStep = 0;
    ledcSetup(_ch1,_prevFreq,PWM_BITS);
    ledcSetup(_ch2,_prevFreq,PWM_BITS);
  }
  
  // SYNTAX: Input value, max PWM, ramptime in ms per 1 PWM increment
  // true = brake active, false = brake in neutral position inactive
  void drive(int controlValue) {
      _controlValue = controlValue;
      //_maxPWM = maxPWM;
      //_rampTime = rampTime;
      //_brake = brake;
      //_neutralBrake = neutralBrake;
      
      if (_invert) {
          _controlValue = map (_controlValue, _minInput, _maxInput, _maxInput, _minInput); // invert driving direction
      }
      
      // Fader (allows to ramp the motor speed slowly up & down) --------------------
      if (_rampTime >= 1) {
          unsigned long currentMillis = millis();
          if (currentMillis - _previousMillis >= _rampTime) {
              // Increase
              if (_controlValue > _controlValueRamp && _controlValueRamp < _maxInput) {
                  _controlValueRamp++;
              }
              // Decrease
              if (_controlValue < _controlValueRamp && _controlValueRamp > _minInput) {
                  _controlValueRamp--;
              }
              _previousMillis = currentMillis;
          }
      }
      else {
          _controlValueRamp = _controlValue;
      }
      
      // H bridge controller -------------------
      _shiftFreq(_controlValueRamp);
      int dutyF = map(_controlValueRamp, _maxNeutral,_maxInput, 0,MAX_DUTY);  // Forward
      int dutyR = map(_controlValueRamp, _minNeutral,_minInput, 0,MAX_DUTY);  // Reverse

      if (_doublePWM) { // Mode with two PWM capable pins (both pins must be PWM capable!) -----
          if (!_brake) { // Coast mode (fast decay)
              if (_controlValueRamp >= _maxNeutral) { // Forward
                  _analogWrite(_ch1, 0);
                  _analogWrite(_ch2, dutyF);
              }
              else if (_controlValueRamp <= _minNeutral) { // Reverse
                  _analogWrite(_ch2, 0);
                  _analogWrite(_ch1, dutyR);
              }
              else { // Neutral
                  _analogWrite(_ch1, 0);
                  _analogWrite(_ch2, 0);
              }
          }
          else { // Brake mode (slow decay)
              if (_controlValueRamp >= _maxNeutral) { // Forward
                  _analogWrite(_ch2, MAX_DUTY);
                  _analogWrite(_ch1, MAX_DUTY - dutyF);
              }
              else if (_controlValueRamp <= _minNeutral) { // Reverse
                  _analogWrite(_ch1, MAX_DUTY);
                  _analogWrite(_ch2, MAX_DUTY - dutyR);
              }
              else { // Neutral
                  if (_neutralBrake) {
                      _analogWrite(_ch1, MAX_DUTY); // Brake in neutral position active
                      _analogWrite(_ch2, MAX_DUTY);
                  }
                  else {
                      _analogWrite(_ch1, 0); // Brake in neutral position inactive
                      _analogWrite(_ch2, 0);
                  }
              }
          }
      }
      else { // Mode with only one PWM capable pin (pin 1 = PWM, pin2 = direction) -----
          // NOTE: the brake is always active in one direction and always inactive in the other!
          // Only use this mode, if your microcontroller does not have enough PWM capable pins!
          // If the brake is active in the wrong direction, simply switch both motor wires and
          // change the "invert" boolean!
          if (_controlValueRamp >= _maxNeutral) { // Forward
              _analogWrite(_ch2, MAX_DUTY);
              _analogWrite(_ch1, MAX_DUTY - dutyF);
          }
          else if (_controlValueRamp <= _minNeutral) { // Reverse
              _analogWrite(_ch2, 0);
              _analogWrite(_ch1, dutyR);
          } else { // Neutral
              _analogWrite(_ch1, MAX_DUTY);
              _analogWrite(_ch2, MAX_DUTY);
          }
      }
  }

};




////////////////////////////////////////////////////////////////////////////////
// class ServoPID{}: PID（比例、積分、微分）制御アルゴリズム（QuickPIDのラッパ）
//  setup(): PID制御のパラメータ変更
//  loop(): PID制御の出力計算
////////////////////////////////////////////////////////////////////////////////
#include <QuickPID.h>

class ServoPID {
public:
  
  float Setpoint, Input, Output;
  float Min, Mean, Max;
  QuickPID* QPID;
  
  ServoPID(void) {
    //
    Setpoint = 0.0F;
    Input = 0.0F;
    Output = 0.0F;
    //
    Mean = 1500;
    Min = 1000 - Mean;
    Max = 2000 - Mean;
    //
    QPID = new QuickPID(&Input, &Output, &Setpoint, 1.0,0.0,0.0, QuickPID::Action::direct);
    QPID->SetMode(QuickPID::Control::automatic);
    QPID->SetOutputLimits(Min,Max);
    QPID->SetSampleTimeUs(1000000/50);
  }
  
  // PID setup
  void setup(float Kp, float Ki, float Kd, int MIN=1000, int MEAN=1500, int MAX=2000, int Hz=50) {
    Min = MIN - MEAN;
    Mean = MEAN;
    Max = MAX - MEAN;
  
    QPID->SetTunings(Kp,Ki,Kd);
    QPID->SetOutputLimits(Min,Max);
    QPID->SetSampleTimeUs(Hz>=50? 1000000/Hz: 1000000/50);
  }
  void setupT(float Kp, float Ti, float Td, int MIN=1000, int MEAN=1500, int MAX=2000, int Hz=50) {
    if (Ti <= 0.0) Ti = 1.0;
    float Ki = Kp/Ti;
    float Kd = Kp*Td;
    setup(Kp,Ki,Kd, MIN,MEAN,MAX,Hz);
  }
  void setupU(float Ku, float Tu, int MIN=1000, int MEAN=1500, int MAX=2000, int Hz=50) {
    if (Tu <= 0.0) Tu = 1.0;
    // Ziegler–Nichols method
    float Ti = 0.50*Tu;
    float Td = 0.125*Tu;
    float Kp = 0.60*Ku;
    float Ki = Kp/Ti;
    float Kd = Kp*Td;
    setup(Kp,Ki,Kd, MIN,MEAN,MAX,Hz);
  }
  
  // PID loop
  float loop(float SP, float PV) {
    // Compute PID
    Setpoint = (SP > 0? SP - Mean: 0.0);
    Input = PV;
    QPID->Compute();
    return SP > 0? Mean + constrain(Output,Min,Max): 0;
  }

  // debug print
  void debug(void) {
    DEBUG.printf("%.2f %.2f %.2f\n", Setpoint,Input,Output);
  }

};





#endif
