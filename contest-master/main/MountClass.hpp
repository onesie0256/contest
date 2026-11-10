#include "head.hpp"
#include <math.h>

class FindMount : public ModeClass
{
public:
  FindMount(){
    this->myMode = Mode::FindMount;
    MM.set(this);
  }

  void whenChanged() override{
    motors.setSpeeds(100, 100);
  }

  void execute(long startTime , long nowTime){
    motors.setSpeeds(100, 100);
    float p = (float)pitch_G;
    float r = (float)roll_G;

    if (angle_G >= 12){
      MM.changeMode(Mode::Climb);
    }
  }
};

class Climb : public ModeClass
{
private:
  float sum_e = 0.0;
  float timePrev;


  float turnTo(float theta_r) {
    // 0 <= theta_r < 360
    // 0 <= direction_G < 360 
    float u;
    float KP = 4.0;
    float TIinv = 2/1000.0;
    float e = theta_r-yaw_G;
    float timeNow = millis();
    if (e<-180) e+=360;
    if (e>=180) e-=360;
    // -180 <= e < 180
    if (abs(e) > 45.0 ) { // |e|>45の時はP制御
    u = KP*e;
    } else {              // |e|<=45 の時はPI制御
      sum_e += TIinv*e*(timeNow-timePrev);
      u = KP*(e+sum_e);   // PI 制御
    }
    if ( u> 180 ) u = 180;
    if ( u<-180 ) u = -180;

    this->timePrev = timeNow;
    return u;
}

public:
  Climb(){
    this->myMode = Mode::Climb;
    MM.set(this);
  }

  void whenChanged() override{
    motors.setSpeeds(0, 0);
    this->sum_e = 0.0;
    this->timePrev = millis();
    tone(3,330);
  }
  //北-90,南90
  void execute(long startTime , long nowTime){

    if (nowTime - startTime > 300){
      noTone(3);
    }

    float diff = turnTo(-90);
    motors.setSpeeds(100-diff, 100+diff);

    if (angle_G < 10){
      MM.changeMode(Mode::Peak);
    }
  }
};

class Peak : public ModeClass
{
public:
  Peak(){
    this->myMode = Mode::Peak;
    MM.set(this);
  }

  void whenChanged() override{
    motors.setSpeeds(30, 30);
    tone(3,330);
  }

  void execute(long startTime , long nowTime){

    if (nowTime - startTime > 50){
      noTone(3);
    }

    if (angle_G > 9){
      MM.changeMode(Mode::Descent);
    }
  }
};

class Descent : public ModeClass
{
private:
  float sum_e = 0.0;
  float timePrev;


  float turnTo(float theta_r) {
    // 0 <= theta_r < 360
    // 0 <= direction_G < 360 
    float u;
    float KP = 4.0;
    float TIinv = 2/1000.0;
    float e = theta_r-yaw_G;
    float timeNow = millis();
    if (e<-180) e+=360;
    if (e>=180) e-=360;
    // -180 <= e < 180
    if (abs(e) > 45.0 ) { // |e|>45の時はP制御
    u = KP*e;
    } else {              // |e|<=45 の時はPI制御
      sum_e += TIinv*e*(timeNow-timePrev);
      u = KP*(e+sum_e);   // PI 制御
    }
    if ( u> 180 ) u = 180;
    if ( u<-180 ) u = -180;

    this->timePrev = timeNow;
    return u;
}

public:
  Descent(){
    this->myMode = Mode::Descent;
    MM.set(this);
  }

  void whenChanged() override{
    motors.setSpeeds(0, 0);
    this->sum_e = 0.0;
    this->timePrev = millis();
    tone(3,330);
  }
  //北-90,南90
  void execute(long startTime , long nowTime){

    if (nowTime - startTime > 300){
      noTone(3);
    }

    float diff = turnTo(-90);
    motors.setSpeeds(100-diff, 100+diff);

    if (angle_G < 10){
      MM.changeMode(Mode::Peak);
    }
  }
};