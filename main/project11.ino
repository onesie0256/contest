/* TO DO
 * 地磁気を用いて南北を判定
 * 記憶できるように定数を設定
 * 0=北,1=南,-1=東西
 */

 #include <Wire.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <LSM303.h>

LSM303 compass;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);

// ===== 結果 =====
int startDir_G = -1;          // 0:北向き 1:南向き -1:東西(または未判定)
bool calibrated_G = false;    // キャリブレーション済みか
float lastHeading_G = 0;      // 直近の判定で得た平均方位[deg](FIELD_NORTH_OFFSET の実測用)

// ===== 設定 =====
const float FIELD_NORTH_OFFSET = 0.0;  // フィールドの「北」が磁北から何度ずれているか(要実測)
const float DIR_TOL = 30.0;            // 北/南とみなす許容幅[deg]
const int   CAL_SPEED = 150;           // キャリブレーション時の回転速度
const unsigned long CAL_TIME_MS = 6000; // キャリブレーション時間[ms](1回転以上になるよう調整)
const int   BUZZER_PIN = 3;            // Zumo シールドのブザー

// -180〜180 に正規化した角度差
float angleDiff(float a, float b) {
  float d = a - b;
  while (d > 180)  d -= 360;
  while (d < -180) d += 360;
  return d;
}

// 2次元キャリブレーション: その場回転して X,Y の最大・最小を探索
void calibrationCompass() {
  compass.m_min.x = 32767;  compass.m_min.y = 32767;  compass.m_min.z = 32767;
  compass.m_max.x = -32768; compass.m_max.y = -32768; compass.m_max.z = -32768;

  unsigned long t0 = millis();
  motors.setSpeeds(CAL_SPEED, -CAL_SPEED);
  while (millis() - t0 < CAL_TIME_MS) {
    compass.read();
    if (compass.m.x < compass.m_min.x) compass.m_min.x = compass.m.x;
    if (compass.m.y < compass.m_min.y) compass.m_min.y = compass.m.y;
    if (compass.m.x > compass.m_max.x) compass.m_max.x = compass.m.x;
    if (compass.m.y > compass.m_max.y) compass.m_max.y = compass.m.y;
    delay(5);
  }
  motors.setSpeeds(0, 0);
  delay(300);  // 停止直後の揺れが収まるのを待つ
}

// 1回分の方位[deg] (0〜360)
float readHeading() {
  compass.read();
  float mx = map(compass.m.x, compass.m_min.x, compass.m_max.x, -128, 127);
  float my = map(compass.m.y, compass.m_min.y, compass.m_max.y, -128, 127);
  float h = atan2(my, mx) * 180 / M_PI;
  if (h < 0) h += 360;
  return h;
}

// スタート位置で静止した状態で呼ぶ
int judgeStartDirection() {
  // 10回分をベクトル平均して、ノイズと360°またぎの影響を避ける
  float sx = 0, sy = 0;
  for (int i = 0; i < 10; i++) {
    float h = readHeading() * M_PI / 180;
    sx += cos(h);
    sy += sin(h);
    delay(5);
  }
  float h = atan2(sy, sx) * 180 / M_PI;
  if (h < 0) h += 360;
  lastHeading_G = h;

  if (fabs(angleDiff(h, FIELD_NORTH_OFFSET))       <= DIR_TOL) return 0;  // 北
  if (fabs(angleDiff(h, FIELD_NORTH_OFFSET + 180)) <= DIR_TOL) return 1;  // 南
  return -1;                                                              // 東西
}

// 結果をブザーで知らせる: 北=1回, 南=2回, 東西=低い長音1回
void beepResult(int dir) {
  if (dir == 0) {
    tone(BUZZER_PIN, 880, 100); delay(200);
  } else if (dir == 1) {
    tone(BUZZER_PIN, 880, 100); delay(200);
    tone(BUZZER_PIN, 880, 100); delay(200);
  } else {
    tone(BUZZER_PIN, 220, 400); delay(500);
  }
}

void setup() {
  Serial1.begin(9600);   // USBシリアルモニタ確認用(不要なら Serial の行は削除してよい)
  Wire.begin();
  compass.init();
  compass.enableDefault();
  pinMode(BUZZER_PIN, OUTPUT);
  Serial1.println("1回目のボタン: キャリブレーション");
}

void loop() {
  // ボタン1回目: キャリブレーション / 2回目以降: 向きの判定
  if (button.getSingleDebouncedPress()) {
    if (!calibrated_G) {
      calibrationCompass();
      calibrated_G = true;
      startDir_G = -1;  // 判定はまだ
      tone(BUZZER_PIN, 1320, 200); delay(300);  // キャリブレーション完了の合図
      Serial1.println("キャリブレーション完了。スタート位置に置いてボタンで判定");
    } else {
      startDir_G = judgeStartDirection();
      beepResult(startDir_G);
      Serial1.print("heading=");
      Serial1.print(lastHeading_G);
      Serial1.print("  startDir_G=");
      Serial1.println(startDir_G);
    }
  }

  // ここに競技用の処理を書く(startDir_G を参照する)
}