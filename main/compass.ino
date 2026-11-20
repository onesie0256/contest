#include <Wire.h>
#include <LSM303.h>
#include <math.h>

#define CALIBRATION_SAMPLES 50  // 地磁気キャリブレーション時のサンプル取得回数
#define CRB_REG_M_2_5GAUSS 0x60 // ±2.5ガウス設定
#define CRA_REG_M_220HZ 0x1C    // 220Hz アップデートレート

LSM303 compass;

Side startSide = Side::Unknown;

static const float COMPASS_NS_TOLERANCE_DEG = 20.0;
static const int COMPASS_SAMPLES = 3;
static const int COMPASS_SAMPLE_MS = 1000;
static const int COMPASS_NEED_SAME = 2;

int norm(float v, float vmin, float vmax)
{
  float span = vmax - vmin;
  if (span < 1) span = 1;
  float n = (v - vmin) * 255.0 / span;
  if (n < 0) n = 0;
  if (n > 255) n = 255;
  return (int)n;
}


// 任意の角度を0以上360未満にそろえる。
float normalizeHeading(float heading)
{
  float normalized = fmod(heading, 360.0f);
  return normalized < 0.0f ? normalized + 360.0f : normalized;
}

float readHeading()
{
  compass.read();

  float x_span = (float)compass.m_max.x - compass.m_min.x;
  float y_span = (float)compass.m_max.y - compass.m_min.y;
  if (x_span <= 0.0f || y_span <= 0.0f){
    return NAN;
  }

  float x_scaled = 2.0 * ((float)compass.m.x - compass.m_min.x) / x_span - 1.0;
  float y_scaled = 2.0 * ((float)compass.m.y - compass.m_min.y) / y_span - 1.0;

  float angle = atan2(y_scaled, x_scaled) * 180 / M_PI;
  if (angle < 0)
    angle += 360;
  return angle;
}

void setupCompass()
{
  Wire.begin();
  compass.init();
  compass.enableDefault();
  compass.writeReg(LSM303::CRB_REG_M, CRB_REG_M_2_5GAUSS);
  compass.writeReg(LSM303::CRA_REG_M, CRA_REG_M_220HZ);
  delay(1000);
  calibrationCompass();
  calibrationStartSide();
}

void calibrationCompass()
{
  unsigned int index;

  LSM303::vector<int16_t> running_min = {32767, 32767, 32767};
  LSM303::vector<int16_t> running_max = {-32767, -32767, -32767};

  // モータを左右逆回転させてその場で回転し、全方位の磁気データを収集
  motors.setLeftSpeed(200);
  motors.setRightSpeed(-200);

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

// スタート位置での向きから自陣側を記憶する（最終的に逆向きを返すことで自分のゴールがどちら側か正しく判定するようにしている）。
Side classifyStartSide(float heading)
{
  if (!isfinite(heading)){
    return Side::Unknown;
  }
  heading = normalizeHeading(heading);
  if (heading <= COMPASS_NS_TOLERANCE_DEG || heading >= 360.0f - COMPASS_NS_TOLERANCE_DEG){
    return Side::North;
  }
  if (fabs(heading - 180.0f) <= COMPASS_NS_TOLERANCE_DEG){
    return Side::South;
  }
  return Side::Unknown;
}

Side judgeStartSide()
{
  int north = 0;
  int south = 0;
  for (int i = 0; i < COMPASS_SAMPLES; i++){
    if (i < COMPASS_SAMPLES - 1){
      delay(COMPASS_SAMPLE_MS);
    }

    Side side = classifyStartSide(readHeading());
    if (side == Side::North) north++;
    if (side == Side::South) south++;
    if (i < COMPASS_SAMPLES - 1){
      delay(COMPASS_SAMPLE_MS);
    }
  }
  if (north >= COMPASS_NEED_SAME) return Side::South;
  if (south >= COMPASS_NEED_SAME) return Side::North;
  return Side::Unknown;
}

void calibrationStartSide()
{
  motors.setSpeeds(0, 0);
  startSide = Side::Unknown;
  while (startSide == Side::Unknown){
    MySerial.println(F("スタート位置にロボットを置き、ボタンを押してください"));
    button.waitForButton();
    startSide = judgeStartSide();
    if (startSide == Side::Unknown){
      MySerial.println(F("判定に失敗しました。向きを直して、もう一度ボタンを押してください"));
    }
  }
  if (startSide == Side::North){
    MySerial.println(F("成功: スタート位置での向きを記憶 (北)"));
  }else{
    MySerial.println(F("成功: スタート位置での向きを記憶 (南)"));
  }
}

void checkBlackLine()
{
  // mode_GはMM.play()後に更新されるため、遷移直後も正しい現在値で判定する。
  switch (MM.getModeNow()){
    case Mode::Forward:
    case Mode::Avoid:
    case Mode::FindMount:
      break;
    default:
      return; //Returnは帰陣用の黒線処理を使う。
  }

  if (color_G == (uint8_t)Color::Black){
    MM.changeMode(Mode::BlackLine);
  }
}

template <typename T> 
int heading(LSM303::vector<T> v)
{
  float x_scaled =  2.0 * (float)(v.x - compass.m_min.x) / ( compass.m_max.x - compass.m_min.x) - 1.0;
  float y_scaled =  2.0 * (float)(v.y - compass.m_min.y) / (compass.m_max.y - compass.m_min.y) - 1.0;

  float angle = atan2(y_scaled, x_scaled) * 180 / M_PI;
  angle -= 90;
  if (angle < 0)
    angle += 360;
  return (int)angle;
}

float averageHeadingLP()
{
  static LSM303::vector<int32_t> avg_g = {
    0, 0, 0
  };

  static LSM303::vector<int32_t> avg_a = {
    0, 0, 0
  };

  compass.read();
  avg_g.x = 0.2 * compass.m.x + 0.8 * avg_g.x;
  avg_g.y = 0.2 * compass.m.y + 0.8 * avg_g.y;

  avg_a.x = 0.2 * compass.a.x + 0.8 * avg_a.x;
  avg_a.y = 0.2 * compass.a.y + 0.8 * avg_a.y;
  avg_a.z = 0.2 * compass.a.z + 0.8 * avg_a.z;

  uint16_t yaw = heading(avg_g);
  yaw_G = yaw;

  pitch_G = atan2f((float)avg_a.x , (float)avg_a.z) * 180 / M_PI;
  roll_G = atan2f((float)avg_a.y , (float)avg_a.z) * 180 / M_PI;

  angle_G = sqrt((pitch_G*pitch_G)+(roll_G*roll_G));


  // avg is the average measure of the magnetic vector.
  return (float)yaw;
}
