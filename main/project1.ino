/*TO DO
 * 最適速度の確認と個体差チェック
 * 250まで試したいが危険なので比較検証は200にする
 * 左右ブレなどの確認をする
 * 加速度が必要かどうかを検討する
 * 
 * result
 * 200でよさそう
 * 電池残量次第でとんでもなく遅くなる
 * （120以下は動かない可能性、38>37>39の順でパワーがあり、39はs=130程度ないと顕著に遅い）
 * 37はやや左にズレる傾向、特に後退する際にひどく左に向く（右寄りに後退する）
 * 38は直進後停止時に若干左を向く後退後停止時に右を向く＝右のモーターの停止が若干遅そう
 * どの機体も、どちらかというと停止しようとするときにブレてそうだったので直進には影響なしと判断する
 * 加速度に関しては必要ないが、段階的減速でぶれる問題が解決するかも
**/
#include <ZumoMotors.h>
#include <Pushbutton.h>

Pushbutton button(ZUMO_BUTTON);
ZumoMotors motors;

const int START_SPEED = 70;          // 初期速度
const int SPEED_STEP = 10;           // 1周ごとの速度の増加量
const int STOP_SPEED = 210;          // この速度になったら停止したままにする
const unsigned long RUN_TIME = 1500; // 走行時間[ms]

int speed = START_SPEED;             // 現在の速度（前進・後進の両方に反映）

void stopMotors() {
  motors.setSpeeds(0, 0);
}

void setup() {}

void loop() {
  // speedが210に達したら、ボタンを押しても動かないよう停止し続ける
  if (speed >= STOP_SPEED) {
    stopMotors();
    while (true) {
      stopMotors();
    }
  }

  // 前進（3秒で自動停止）
  motors.setSpeeds(speed, speed);
  delay(RUN_TIME);
  stopMotors();

  // ボタンが押されるまで待機
  button.waitForButton();
  delay(500);   // 手が離れる時間を確保（任意）

  // 後進（3秒で自動停止）
  motors.setSpeeds(-speed, -speed);
  delay(RUN_TIME);
  stopMotors();

  // 次の前進に入る前にもう一度ボタン待ち（不要なら削除）
  button.waitForButton();
  delay(500);

  // 1周終わったので速度を上げる
  speed += SPEED_STEP;
}