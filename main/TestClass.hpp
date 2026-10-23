#include "head.hpp"

class A : public ModeClass
{
private:
  bool isFirst = true;
  int f = 220;

public:
  A(){
    this->myMode = Mode::Mode0_A;
    MM.set(this);
  }

  void whenChanged() override{
    isFirst = true; //
    f += 55;
    //MySerial.println("A");
  }

  void execute(long startTime , long nowTime){
    if (isFirst){
      //tone(3,this->f);
      //MySerial.println("tone");
      isFirst = false;
    }

    // MySerial.print("A:");
    // MySerial.print(startTime);
    // MySerial.print(" ");
    // MySerial.println(nowTime);

    if (nowTime - startTime >= 500){
      //noTone(3);
      MM.changeMode(Mode::Forward);
    }
  }
};

class B : public ModeClass
{
public:
  B(){
    this->myMode = Mode::Forward;
    MM.set(this);
  }

  void whenChanged() override{
    noTone(3);
    //MySerial.println("B");
  }
  void execute(long startTime , long nowTime){
    // if (nowTime - startTime >= 2000){
    //   MM.changeMode(Mode::Mode0_A);
    // }
    ;
  }
};