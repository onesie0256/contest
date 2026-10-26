#include <Arduino.h>
#include <Wire.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <Adafruit_TCS34725.h>
#include <LSM303.h>
#include "head.hpp"
#include "TestClass.hpp"
#include "PacketType.h"

/* _Gが付いてる変数の内容がprocessingに送られる! */
Mode mode_G = Mode::Mode0_A;
uint8_t r_G = 0;
uint8_t g_G = 0;
uint8_t b_G = 0;
uint8_t color_G = 0;
uint8_t distance_G = 0;
uint16_t pitch_G = 0;
uint16_t roll_G = 0;
uint16_t yaw_G = 0;
uint8_t isStacking_G = 0;

A a;
Pro1 pro1;
Pro4 pro4;
Pro5 pro5;

#define CALIBRATION_SAMPLES 50  // 地磁気キャリブレーション時のサンプル取得回数
#define CRB_REG_M_2_5GAUSS 0x60 // ±2.5ガウス設定
#define CRA_REG_M_220HZ 0x1C    // 220Hz アップデートレート

ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_60X);
LSM303 compass;

float r, g, b;                                                            //生のRGB値
float rmin = 0, gmin = 0, bmin = 0, rmax = 255, gmax = 255, bmax = 255;   //calibrationColorSensor()で更新される
int nr, ng, nb;                                                           //0〜255に規格化したRGB値
FieldColor colorNow = FieldColor::Other;                                  //最後に判定した色

const int BLACK_TH = 50;

int norm(float v, float vmin, float vmax)
{
  float span = vmax - vmin;
  if (span < 1) span = 1;
  float n = (v - vmin) * 255.0 / span;
  if (n < 0) n = 0;
  if (n > 255) n = 255;
  return (int)n;
}

FieldColor measureColor()
{
  tcs.getRGB(&r, &g, &b);
  nr = norm(r, rmin, rmax);
  ng = norm(g, gmin, gmax);
  nb = norm(b, bmin, bmax);

  if (nr > 200 && ng > 200 && nb > 200)      colorNow = FieldColor::White;
  else if (nr < BLACK_TH && ng < BLACK_TH && nb < BLACK_TH) colorNow = FieldColor::Black;
  else if (nr > 100 && ng < 50 && nb < 50)   colorNow = FieldColor::Red;
  else if (nr < 50 && ng < 50 && nb > 70)    colorNow = FieldColor::Blue;
  else                                       colorNow = FieldColor::Other;

  return colorNow;
}

float readHeading()
{
  compass.read();

  float x_scaled = 2.0 * (float)(compass.m.x - compass.m_min.x) / (compass.m_max.x - compass.m_min.x) - 1.0;
  float y_scaled = 2.0 * (float)(compass.m.y - compass.m_min.y) / (compass.m_max.y - compass.m_min.y) - 1.0;

  float angle = atan2(y_scaled, x_scaled) * 180 / M_PI;
  if (angle < 0)
    angle += 360;
  return angle;
}

void calibrationColorSensor()
{
  rmin = gmin = bmin = 30000;
  rmax = gmax = bmax = 0;

  unsigned long startT = millis();
  while (millis() - startT < 2000) {
    motors.setSpeeds(120, 120); // 120の速度で2秒前進
    tcs.getRGB(&r, &g, &b);
    if (r < rmin) rmin = r;
    if (r > rmax) rmax = r;
    if (g < gmin) gmin = g;
    if (g > gmax) gmax = g;
    if (b < bmin) bmin = b;
    if (b > bmax) bmax = b;
  }
  motors.setSpeeds(0, 0); // 停止
}

void setupCompass()
{
  compass.init();
  compass.enableDefault();
  compass.writeReg(LSM303::CRB_REG_M, CRB_REG_M_2_5GAUSS);
  compass.writeReg(LSM303::CRA_REG_M, CRA_REG_M_220HZ);
  delay(1000);
}

void calibrationCompass()
{
  unsigned int index;

  LSM303::vector<int16_t> running_min = {32767, 32767, 32767};
  LSM303::vector<int16_t> running_max = {-32767, -32767, -32767};

  // モータを左右逆回転させてその場で回転し、全方位の磁気データを収集
  motors.setLeftSpeed(200);
  motors.setRightSpeed(-200);

  for (index = 0; index < CALIBRATION_SAMPLES; index++)
  {
    compass.read();
    running_min.x = min(running_min.x, compass.m.x);
    running_min.y = min(running_min.y, compass.m.y);
    running_min.z = min(running_min.z, compass.m.z);

    running_max.x = max(running_max.x, compass.m.x);
    running_max.y = max(running_max.y, compass.m.y);
    running_max.z = max(running_max.z, compass.m.z);
    delay(50);
  }

  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);

  // 最大値・最小値をセット
  compass.m_max.x = running_max.x;
  compass.m_max.y = running_max.y;
  compass.m_max.z = running_max.z;
  compass.m_min.x = running_min.x;
  compass.m_min.y = running_min.y;
  compass.m_min.z = running_min.z;
}

void calibrateAll()
{
  // カラーセンサ
  MySerial.println(F("ロボットを白と黒が通る位置に置き、ボタンを押してください"));
  button.waitForButton();
  delay(500);
  MySerial.println(F("カラーキャリブレーション中（120で2秒前進します）..."));
  calibrationColorSensor();
  MySerial.println(F("カラーキャリブレーション完了"));

  // 地磁気センサ
  MySerial.println(F("ボタンを押すと、その場で回転して地磁気キャリブレーションをします"));
  button.waitForButton();
  delay(500);
  calibrationCompass();
  MySerial.println(F("地磁気キャリブレーション完了"));
}

void checkBlackLine()
{
  if (mode_G != Mode::Forward){
    return;
  }

  if (measureColor() == FieldColor::Black){
    MM.changeMode(Mode::BlackLine);
  }
}

void setup()
{
  MySerial.begin(9600);
  while (!MySerial){
    delay(1);
  }
  a = A();
  //b = B();
  //MySerial.println("start");

  Wire.begin();
  tcs.begin();
  setupCompass();
  calibrateAll();

  pinMode(13,OUTPUT);
}

void loop()
{
  checkBlackLine();
  mode_G = MM.play();
  receive();
  delay(50);
}