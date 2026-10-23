#include <Wire.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <Adafruit_TCS34725.h>

ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_60X);

// 色定義
#define WHITE 0
#define BLACK 1
#define RED   2
#define BLUE  3
#define OTHER 4

float r, g, b;
float rmin = 0, gmin = 0, bmin = 0, rmax = 255, gmax = 255, bmax = 255;
int nr, ng, nb;
int color = OTHER;
int prevColor = -1; // 前回認識していた色を保持する変数（変化検知用）

const int BLACK_TH = 50;

// 0〜255への規格化関数
int norm(float v, float vmin, float vmax) {
  float span = vmax - vmin;
  if (span < 1) span = 1;
  float n = (v - vmin) * 255.0 / span;
  if (n < 0) n = 0; if (n > 255) n = 255;
  return (int)n;
}

// カラー測定と判定
void measureColor() {
  tcs.getRGB(&r, &g, &b);
  nr = norm(r, rmin, rmax);
  ng = norm(g, gmin, gmax);
  nb = norm(b, bmin, bmax);
  
  if (nr > 200 && ng > 200 && nb > 200) color = WHITE;
  else if (nr < BLACK_TH && ng < BLACK_TH && nb < BLACK_TH) color = BLACK;
  else if (nr > 100 && ng < 50 && nb < 50) color = RED;
  else if (nr < 50 && ng < 50 && nb > 70) color = BLUE;
  else color = OTHER;
}

// 一定間隔タイマー用関数
bool tick(unsigned long ms) {
  static unsigned long last = 0;
  if (millis() - last >= ms) { last = millis(); return true; }
  return false;
}

// 120の速度で2秒間前進しながら白と黒のRGB最小・最大を自動計測する関数
void calibrationColorSensor() {
  rmin = gmin = bmin = 30000;
  rmax = gmax = bmax = 0;
  
  unsigned long startT = millis();
  while (millis() - startT < 2000) {
    motors.setSpeeds(120, 120); // 120の速度で2秒前進
    tcs.getRGB(&r, &g, &b);
    if (r < rmin) rmin = r; if (r > rmax) rmax = r;
    if (g < gmin) gmin = g; if (g > gmax) gmax = g;
    if (b < bmin) bmin = b; if (b > bmax) bmax = b;
  }
  motors.setSpeeds(0, 0); // 停止
}

void setup() {
  Serial1.begin(9600);
  Wire.begin();
  tcs.begin();

  // --- 【問題4のキャリブレーション＆スタート手順】 ---
  Serial1.println(F("ロボットを白と黒が通る位置に置き、ボタンを押してください"));
  button.waitForButton(); // 1回目のボタン：キャリブレーション開始
  delay(500);

  Serial1.println(F("キャリブレーション中（120で2秒前進します）..."));
  calibrationColorSensor(); // 自動キャリブレーション実行（2秒前進）

  Serial1.println(F("キャリブレーション完了！もう一度ボタンを押すと走行を開始します"));
  button.waitForButton(); // 2回目のボタン：走行開始待ち
  delay(500);
  Serial1.println(F(">>> 走行開始（問題4）"));

  // 最初の反転動作（必要に応じて調整）
  motors.setSpeeds(200, -200);
  delay(1000); 
  motors.setSpeeds(0, 0);
  delay(500);
}

void loop() {
  // ==========================================
  // 【問題4】カップ保持状態でのゴール探索・帰還
  // ==========================================
  measureColor();

  // 1. 約200msごとに現在のRGB値を出力
  if (tick(200)) {
    Serial1.print(F("RGB -> R: ")); Serial1.print(r);
    Serial1.print(F("\tG: ")); Serial1.print(g);
    Serial1.print(F("\tB: ")); Serial1.println(b);
  }

  // 2. 認識している色が切り替わった瞬間に出力
  if (color != prevColor) {
    Serial1.print(F(">>> 【色変更検知】 新たに認識した色: "));
    if (color == WHITE) Serial1.println(F("白 (WHITE)"));
    else if (color == BLACK) Serial1.println(F("黒 (BLACK)"));
    else if (color == RED) Serial1.println(F("赤 (RED)"));
    else if (color == BLUE) Serial1.println(F("青 (BLUE)"));
    else Serial1.println(F("その他 (OTHER)"));
    
    prevColor = color;
  }

  // 基本の直進
  motors.setSpeeds(120, 120);

  // 黒線（黒枠）を検知したときの回避動作
  if (color == BLACK) {
    motors.setSpeeds(-150, 150);
    delay(1100);
    motors.setSpeeds(120, 120);
    delay(70);
    motors.setSpeeds(0, 0);
    delay(10);

    measureColor();
    if (color == BLACK) {
      motors.setSpeeds(150, -150);
      delay(2000);
    }
    delay(200);
  }

  // ゴールライン（赤または青）を検知したとき
  if (color == RED || color == BLUE) {
    motors.setSpeeds(0, 0);
    delay(200);
    motors.setSpeeds(-150, -150);
    while (true); // 停止
  }
}