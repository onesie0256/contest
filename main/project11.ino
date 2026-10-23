#include <Wire.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <Adafruit_TCS34725.h>
#include <LSM303.h>

#define CALIBRATION_SAMPLES 50  // キャリブレーション時のサンプル取得回数
#define CRB_REG_M_2_5GAUSS 0x60 // ±2.5ガウス設定[cite: 3]
#define CRA_REG_M_220HZ 0x1C    // 220Hz アップデートレート[cite: 3]

ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_60X);
LSM303 compass;

void setupCompass()
{
  compass.init();
  compass.enableDefault();
  compass.writeReg(LSM303::CRB_REG_M, CRB_REG_M_2_5GAUSS);
  compass.writeReg(LSM303::CRA_REG_M, CRA_REG_M_220HZ);
  delay(1000);
}

void calibrationCompass()
{
  unsigned int index;
  int motorL, motorR;

  LSM303::vector<int16_t> running_min = {32767, 32767, 32767};
  LSM303::vector<int16_t> running_max = {-32767, -32767, -32767};

  // モータを左右逆回転させてその場で回転し、全方位の磁気データを収集
  motorL = 200;
  motorR = -200;
  motors.setLeftSpeed(motorL);
  motors.setRightSpeed(motorR);

  for (index = 0; index < CALIBRATION_SAMPLES; index++)
  {
    compass.read();
    running_min.x = min(running_min.x, compass.m.x);
    running_min.y = min(running_min.y, compass.m.y);
    running_min.z = min(running_min.z, compass.m.z);

    running_max.x = max(running_max.x, compass.m.x);
    running_max.y = max(running_max.y, compass.m.y);
    running_max.z = max(running_max.z, compass.m.z);
    delay(50);
  }

  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);

  // 最大値・最小値をセット
  compass.m_max.x = running_max.x;
  compass.m_max.y = running_max.y;
  compass.m_max.z = running_max.z;
  compass.m_min.x = running_min.x;
  compass.m_min.y = running_min.y;
  compass.m_min.z = running_min.z;
}

template <typename T>
float getHeadingAngle(LSM303::vector<T> v)
{
  float x_scaled = 2.0 * (float)(v.x - compass.m_min.x) / (compass.m_max.x - compass.m_min.x) - 1.0;
  float y_scaled = 2.0 * (float)(v.y - compass.m_min.y) / (compass.m_max.y - compass.m_min.y) - 1.0;

  float angle = atan2(y_scaled, x_scaled) * 180 / M_PI;
  if (angle < 0)
    angle += 360;
  return angle;
}

void setup()
{
  Serial1.begin(9600);
  Wire.begin();
  tcs.begin();

  if (EXEC_MODE == 11)
  {
    setupCompass();
  }

  button.waitForButton();
  delay(500);

  Serial1.println(F("=== 自動キャリブレーション開始 ==="));
  calibrationCompass(); // サンプルコードの自動回転キャリブレーションを実行
  Serial1.println(F("キャリブレーション完了。0.5秒待機..."));
  delay(500);
  Serial1.println(F("待機状態：ボタンを押すと方角を判定します"));
}

void loop()
{

  // ボタンを押すたびに現在の向いている方角を更新してSerial1に出力
  if (button.getSingleDebouncedPress())
  {
    compass.read();
    float heading = getHeadingAngle(compass.m); // 0〜360度の正確な角度

    int judgmentValue = -1; // その他

    // 南北判定（±20°許容）
    // 北：0°付近 (0°〜20° または 340°〜360°)
    if (heading <= 20.0 || heading >= 340.0)
    {
      judgmentValue = 0; // 北
    }
    // 南：180°付近 (160°〜200°)
    else if (heading >= 160.0 && heading <= 200.0)
    {
      judgmentValue = 1; // 南
    }

    // XBee (Serial1) で分かりやすく出力
    Serial1.print(F("Heading: "));
    Serial1.print(heading);
    Serial1.print(F(" deg, Judgment (North=0, South=1, Other=-1): "));
    Serial1.println(judgmentValue);
  }
}