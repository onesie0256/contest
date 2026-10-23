/**
仮のコードです.
*/
#include "head.hpp"
#include "TestClass.hpp"
#include "PacketType.h"
#include "ResetClass.hpp"
#include <Pushbutton.h>
#include <ZumoMotors.h>

ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);

/* _Gが付いてるこれらの変数の内容がprocessingに送られる! */
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
Reset r;

void setup()
{
  MySerial.begin(9600);
  while (!MySerial){
    delay(1);
  }
  a = A();
  b = B();
  r = Reset();

  button.waitForButton(); 
  auto_calib();
  tone(3,220);
  delay(500);
  noTone(3);
  //MySerial.println("start");
  pinMode(13,OUTPUT);
}

void loop()
{
  getColor();
  color_G = (uint8_t)Nearest_Neighbor(); 
   if (button.isPressed() == true) { //ユーザボタンが押されている間は後転
      // digitalWrite(13,HIGH);
      // delay(1000);
      // digitalWrite(13,LOW); 
    MM.changeModeForce(Mode::Reset); 
  }

  mode_G = MM.play();
  receive();
  delay(50);
}