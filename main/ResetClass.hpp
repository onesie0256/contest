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
  }

  void execute(long startTime , long nowTime){
    if (nowTime-startTime <= coolTime){
      return;
    }

    noTone(3);

    switch(color_G){
      case (uint8_t)(Color::Yellow) : {
        MM.changeMode(Mode::Mode0_A); break;
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