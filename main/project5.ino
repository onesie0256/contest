/*TO DO
 *黒線の認識
 *180°回転すればコースアウトしない
 *若干だけ後退してから回転
 *回転する速度と時間を最適なものをみつける(もしくは組み合わせ)
**/

#include <Wire.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <LSM303.h>

LSM303 compass;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);

// ===== ここだけ変える =====
const int ROT_SPEED = 150;              // 回転速度
const unsigned long ROT_TIME_MS = 600;  // 回転時間[ms]
const int ROT_DIR = 1;                  // 1=左回り, -1=右回り
const int HSIGN = 1;                    // 回転量が「実際の 360-回転量」で出たら -1 にする

bool calibrated = false;

// キャリブレーション: その場で6秒回って X,Y の最大・最小を探す
void calibrationCompass() {
  compass.m_min.x = compass.m_min.y = 32767;
  compass.m_max.x = compass.m_max.y = -32768;
  unsigned long t0 = millis();
  motors.setSpeeds(150, -150);
  while (millis() - t0 < 6000) {
    compass.read();
    if (compass.m.x < compass.m_min.x) compass.m_min.x = compass.m.x;
    if (compass.m.y < compass.m_min.y) compass.m_min.y = compass.m.y;
    if (compass.m.x > compass.m_max.x) compass.m_max.x = compass.m.x;
    if (compass.m.y > compass.m_max.y) compass.m_max.y = compass.m.y;
    delay(5);
  }
  motors.setSpeeds(0, 0);
  delay(300);
}

// 停止中に10回読んで平均した方位[deg] (0〜360)
float heading() {
  float sx = 0, sy = 0;
  for (int i = 0; i < 10; i++) {
    compass.read();
    float mx = map(compass.m.x, compass.m_min.x, compass.m_max.x, -128, 127);
    float my = map(compass.m.y, compass.m_min.y, compass.m_max.y, -128, 127);
    float h = atan2(my, mx);
    sx += cos(h);
    sy += sin(h);
    delay(5);
  }
  float h = atan2(sy, sx) * 180 / M_PI;
  if (h < 0) h += 360;
  return h;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  compass.init();
  compass.enableDefault();
  Serial.println("1回目のボタン: キャリブレーション / 2回目以降: 1回回す");
}

void loop() {
  if (!button.getSingleDebouncedPress()) return;

  if (!calibrated) {
    calibrationCompass();
    calibrated = true;
    Serial.println("準備完了");
    return;
  }

  float before = heading();
  motors.setSpeeds(-ROT_DIR * ROT_SPEED, ROT_DIR * ROT_SPEED);
  delay(ROT_TIME_MS);
  motors.setSpeeds(0, 0);
  delay(400);                            // 停止後の揺れが収まるのを待つ
  float after = heading();

  float rot = fmod(HSIGN * ROT_DIR * (after - before) + 720, 360);
  Serial.print("speed=");  Serial.print(ROT_SPEED);
  Serial.print(" time=");  Serial.print(ROT_TIME_MS);
  Serial.print(" dir=");   Serial.print(ROT_DIR);
  Serial.print(" rot[deg]="); Serial.println(rot, 1);
}