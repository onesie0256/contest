#pragma once
#include "head.hpp"

class Reset : public ModeClass
{
private:
static constexpr long coolTime = 1000;
public:
  Reset(){
    this->myMode = Mode::Reset;
    MM.set(this);
  }

  void whenChanged() override{
    tone(3,600);
    motors.setSpeeds(0, 0);
  }

  void execute(long startTime , long nowTime){
    if (nowTime-startTime <= coolTime){
      return;
    }

    noTone(3);

    switch(color_G){
      case (uint8_t)(Color::Yellow) : {
        MM.changeModeForce(Mode::Mode0_A);
        // digitalWrite(13,HIGH);
        // delay(1000);
        // digitalWrite(13,LOW);
        break;
      }

      case (uint8_t)(Color::Green) : {
        MM.changeModeForce(Mode::FindMount);
        // digitalWrite(13,HIGH);
        // delay(1000);
        // digitalWrite(13,LOW);
        break;
      }
      
      default : break;
    }
  }
};

extern Reset r;