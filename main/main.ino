#include <Wire.h>
#include <LSM303.h> //何故かここを消すとコンパイルエラー
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include "head.hpp"
#include "Ultrasonic.hpp" 
#include "TestClass.hpp"
#include "PacketType.h"
#include "ResetClass.hpp"
#include "MountClass.hpp"

/* _Gが付いてるこれらの変数の内容がprocessingに送られる! */
Mode mode_G = Mode::Mode0_A;
uint8_t r_G = 0;
uint8_t g_G = 0;
uint8_t b_G = 0;
uint8_t color_G = 0;
uint8_t distance_G = 0;
int16_t pitch_G = 0;
int16_t roll_G = 0;
int16_t yaw_G = 0;
uint8_t isStacking_G = 0;
uint16_t angle_G = 0;


A a;
Reset r;
FindMount fm;
Climb c;
 Pro1 pro1;
 Pro2 pro2;
 Pro3 pro3;
 Pro4 pro4;
 Pro5 pro5;
 Pro6 pro6;
 Pro7 pro7;


void setup()
{
  MySerial.begin(9600);
  while (!MySerial){
    delay(1);
  }
   pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);

  // a = A();
  // r = Reset();
  // fm = FindMount();
  // c = Climb();

  button.waitForButton(); 
  auto_calib();
  tone(3,220);
  delay(500);
  noTone(3);

  button.waitForButton(); 
  setupCompass();
  calibrationCompass();
  tone(3,220);
  delay(500);
  noTone(3);

  pinMode(13,OUTPUT);
}

void loop()
{
  getColor();
  averageHeadingLP();

  color_G = (uint8_t)Nearest_Neighbor();

  // ボタンが押されたとき
  if (button.getSingleDebouncedPress()) {

    // Mode0_AならForwardへ
    if (mode_G == Mode::Mode0_A) {
      MM.changeModeForce(Mode::Forward);
    }
    // Mode0_A以外ならResetへ
    else {
      MM.changeModeForce(Mode::Reset);
    }
  }

  checkBlackLine();

  mode_G = MM.play();

  receive();

  delay(50);
}