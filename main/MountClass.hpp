#include "head.hpp"

class FindMount : public ModeClass
{
private:
  Timer timer;

public:
  FindMount(){
    this->myMode = Mode::FindMount;
    MM.set(this);
  }

  void whenChanged() override{
    ;
  }

  void execute(long startTime , long nowTime){
    if (nowTime - startTime >= 5000){
      //noTone(3);
      MM.changeMode(Mode::Forward);
    }
  }
};