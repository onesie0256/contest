/**
仮のコードです.
*/
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
B b;

void setup()
{
  MySerial.begin(115200);
  while (!MySerial){
    delay(1);
  }
  a = A();
  b = B();
  //MySerial.println("start");
  pinMode(13,OUTPUT);
}

void loop()
{
  mode_G = MM.play();
  receive();
  delay(50);
}