#include "Adafruit_TCS34725.h"
#include "head.hpp"
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_60X);
#define cal_time 3000

unsigned int r_min = 30000, g_min = 30000, b_min = 30000;
unsigned int r_max = 0, g_max = 0, b_max = 0;


#define max_colors 8  //NN: 識別したい色の数
unsigned int ave_colors[max_colors][3] = {
  { 255, 255, 255 },  //白識別：red, green, blue 値
  { 34, 41, 49 },     //黒識別：red, green, blue 値
  { 123, 3, 8 },      //赤
  { 48, 110, 54 },    //緑
  { 3, 39, 122 },     //青
  { 35, 139, 221 },   //シアン
  { 119, 5, 49 },     //マゼンタ
  { 255, 255, 93 }    //イエロー
};

void getColor()
{
  float red,green,blue;
  tcs.getRGB(&red, &green, &blue);
  r_G = map(int(red), r_min, r_max, 0, 255);
  g_G = map(int(green), g_min, g_max, 0, 255);
  b_G = map(int(blue), b_min, b_max, 0, 255);
}

void auto_calib() {

  unsigned long timeInit;
  timeInit = millis();

  float red,green,blue;
  motors.setSpeeds(60, 60); 

  while (1) {
    tcs.getRGB(&red, &green, &blue);

  red = map(int(red), r_min, r_max, 0, 255);
  //ここに Green 値・Blue 値の処理を記述
  green = map(int(green), g_min, g_max, 0, 255);
  blue = map(int(blue), b_min, b_max, 0, 255);
    tcs.getRGB(&red, &green, &blue);
    if (red > r_max) r_max = red;
    if (red < r_min) r_min = red;
    Serial.print("r_min, r_max = ");
    Serial.print(r_min);
    Serial.print(", ");
    Serial.println(r_max);
    //ここに Green 値・Blue 値の処理を記述
    if (green > g_max) g_max = green;
    if (green < g_min) g_min = green;
    Serial.print("g_min, g_max = ");
    Serial.print(g_min);
    Serial.print(", ");
    Serial.println(g_max);

    if (blue > b_max) b_max = blue;
    if (blue < b_min) b_min = blue;
    Serial.print("b_min, b_max = ");
    Serial.print(b_min);
    Serial.print(", ");
    Serial.println(b_max);
    delay(10);
    if (millis() - timeInit > cal_time) {
      motors.setSpeeds(0, 0);
      return;
    }
  }
}

Color Nearest_Neighbor() {
  int color = -1;
  float minDistance = 9999999;
  float distance;

  for (int i = 0; i < max_colors; i++) {  // i は設定したカラーのラベル
    distance = sqrt((float)((r_G - ave_colors[i][0]) * (r_G - ave_colors[i][0])) + (float)((g_G - ave_colors[i][1]) * (g_G - ave_colors[i][1])) + (float)((b_G - ave_colors[i][2]) * (b_G - ave_colors[i][2])));

    if (distance < minDistance) {  //最短距離の判定
      minDistance = distance;
      color = i;
    }
  }

  Color rtn;

  switch (color) {
    case 0: rtn = Color::White; break;
    case 1: rtn = Color::Black; break;
    case 2: rtn = Color::Red; break;
    case 3: rtn = Color::Green; break;
    case 4: rtn = Color::Blue; break;
    case 5: rtn = Color::Cyan; break;
    case 6: rtn = Color::Magenta; break;
    case 7: rtn = Color::Yellow; break;
    default: rtn = Color::UNKNOWN;
  }

  return rtn;
}