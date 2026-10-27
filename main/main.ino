#include <Wire.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include "head.hpp"
#include "TestClass.hpp"
#include "PacketType.h"
#include "ResetClass.hpp"


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
Reset r;
Pro1 pro1;
Pro4 pro4;
Pro5 pro5;


void setup()
{
  MySerial.begin(9600);
  while (!MySerial){
    delay(1);
  }
  a = A();
  r = Reset();

  button.waitForButton(); 
  auto_calib();
  tone(3,220);
  delay(500);
  noTone(3);

  setupCompass();
  calibrationCompass();

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

  checkBlackLine();
  mode_G = MM.play();
  receive();
  delay(50);
}