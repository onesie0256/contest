#include <Wire.h>
#include <LSM303.h>

#define CALIBRATION_SAMPLES 50  // 地磁気キャリブレーション時のサンプル取得回数
#define CRB_REG_M_2_5GAUSS 0x60 // ±2.5ガウス設定
#define CRA_REG_M_220HZ 0x1C    // 220Hz アップデートレート

LSM303 compass;

const int BLACK_TH = 50;

int norm(float v, float vmin, float vmax)
{
  float span = vmax - vmin;
  if (span < 1) span = 1;
  float n = (v - vmin) * 255.0 / span;
  if (n < 0) n = 0;
  if (n > 255) n = 255;
  return (int)n;
}


float readHeading()
{
  compass.read();

  float x_scaled = 2.0 * (float)(compass.m.x - compass.m_min.x) / (compass.m_max.x - compass.m_min.x) - 1.0;
  float y_scaled = 2.0 * (float)(compass.m.y - compass.m_min.y) / (compass.m_max.y - compass.m_min.y) - 1.0;

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

void checkBlackLine()
{
  if (mode_G != Mode::Forward){
    return;
  }

  if (color_G == (uint8_t)Color::Black){
    MM.changeMode(Mode::BlackLine);
  }
}