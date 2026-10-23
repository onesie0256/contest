#include "head.hpp"

class Reset : public ModeClass
{
private:
const long coolTime = 1000;
public:
  Reset(){
    this->myMode = Mode::Reset;
    MM.set(this);
  }

  void whenChanged() override{
    tone(3,600);
  }

  void execute(long startTime , long nowTime){
    if (nowTime-startTime >= coolTime){
      return;
    }

    noTone(3);

    switch(color_G){
      case (uint8_t)(Color::Magenta) : MM.changeMode(Mode::Mode0_A); break;
      default : break;
    }
  }
};