/*TO DO
 *最適速度の確認と個体差チェック
 *250まで試したいが危険なので比較検証は200にする
 *左右ブレなどの確認をする
 *加速度が必要かどうかを検討する
**/
#include <ZumoMotors.h>
#include <Pushbutton.h>

Pushbutton button(ZUMO_BUTTON);
ZumoMotors motors;

const int SPEED = 100;              // 速度はここだけ変えれば前進・後進の両方に反映
const unsigned long RUN_TIME = 3000; // 走行時間[ms]

void stopMotors() {
  motors.setSpeeds(0, 0);
}

void setup() {}

void loop() {
  // 前進（3秒で自動停止）
  motors.setSpeeds(SPEED, SPEED);
  delay(RUN_TIME);
  stopMotors();

  // ボタンが押されるまで待機
  button.waitForButton();
  delay(500);   // 手が離れる時間を確保（任意）

  // 後進（3秒で自動停止）
  motors.setSpeeds(-SPEED, -SPEED);
  delay(RUN_TIME);
  stopMotors();

  // 次の前進に入る前にもう一度ボタン待ち（不要なら削除）
  button.waitForButton();
  delay(500);
}