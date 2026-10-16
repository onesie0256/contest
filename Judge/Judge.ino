#include <ZumoMotors.h>
#include <Pushbutton.h>

const int trig = 2;                       // 超音波センサのTrig端子
const int echo = 4;                       // 超音波センサのEcho端子

ZumoMotors motors;                        // モータ制御用オブジェクト
Pushbutton button(ZUMO_BUTTON);           // Zumoのボタン

int mode = 0;                             // 0:Search、1:Judge、2:Done
unsigned long judgeStartTime = 0;         // Judgeモード開始時刻
unsigned long measureStartTime = 0;  // 距離計測の開始時刻
bool measuring = false;              // 計測中かどうか
int previousDist = -1;                    // 前回の有効な距離
const int MOVE_THRESHOLD = 1;             // 距離が1cm以上変化したら動いたと判定

int distance() {
  unsigned long interval;
  int dist;

  digitalWrite(trig, HIGH);               // 超音波を送信
  delayMicroseconds(10);                  // 10マイクロ秒待つ
  digitalWrite(trig, LOW);                // 送信を終了

  interval = pulseIn(echo, HIGH, 23071);  // 反射波を受信（タイムアウトあり）

  if (interval == 0) {                    // 受信できなかった場合
    return -1;                            // 無効な距離として返す
  }

  dist = (0.61 * 25 + 331.5) * interval / 10000 / 2; // 距離(cm)を計算
  return dist;
}

void setup() {
  Serial.begin(9600);                     // シリアル通信を開始
  pinMode(trig, OUTPUT);                  // Trigを出力に設定
  pinMode(echo, INPUT);                   // Echoを入力に設定

  motors.setSpeeds(0, 0);                 // モータを停止
  button.waitForButton();                 // ボタンが押されるまで待機
}

void loop() {
  int dist = distance();                  // 現在の距離を測定

  Serial.print("Mode: ");                 // 現在のモードを表示
  Serial.print(mode);
  Serial.print("  Distance: ");
  if (dist >= 0) {
    Serial.print(dist);
    Serial.println(" cm");
  } else {
    Serial.println("測定失敗");
  }

  if (mode == 0) {                        // Searchモード
    motors.setSpeeds(0, 0);               // 手動で動かすためモータは停止

    if (dist >= 0 && dist <= 15) {        // 15cm以内に物体を検出
  mode = 1;                           // Judgeモードへ移行
  judgeStartTime = millis();          // Judgeモード開始時刻を記録
  measuring = false;                  // まだ計測を開始しない
  previousDist = -1;                  // 前回の距離をリセット
  Serial.println("Judge mode start");
}
  }
  else if (mode == 1) {                  // Judgeモード
  motors.setSpeeds(0, 0);              // 判定中はモータを停止

  if (!measuring) {                    // まだ計測を開始していない場合
    if (millis() - judgeStartTime >= 1000) { // Judge開始から1秒経過
      measuring = true;                // 計測を開始
      measureStartTime = millis();     // 計測開始時刻を記録
      previousDist = dist;             // 計測開始時の距離を保存
      Serial.println("Distance measurement start");
    }
  }
  else {                               // 計測開始後
    if (dist >= 0 && previousDist >= 0) { // 距離が正常に測定できた場合
      if (abs(dist - previousDist) >= MOVE_THRESHOLD) { // 1cm以上の変化を確認
        Serial.println("ロボット");     // 動いたと判定
        mode = 2;                      // Doneモードへ移行
      }
    }

    if (mode == 1) {                   // まだ動いたと判定していない場合
      if (dist >= 0) {
        previousDist = dist;           // 次の測定との比較用に更新
      }

      if (millis() - measureStartTime >= 3000) { // 計測開始から3秒経過
        Serial.println("コップ");       // 動かなかったと判定
        mode = 2;                      // Doneモードへ移行
      }
    }
  }
}
  else if (mode == 2) {                   // Doneモード
    motors.setSpeeds(0, 0);               // モータを停止したままにする
  }

  delay(3000);                              // 約100ms間隔で繰り返し測定
}