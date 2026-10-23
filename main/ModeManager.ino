#include "head.hpp"

ModeManager::ModeManager()
{
  ;
}

bool ModeManager::set(ModeClass *target)
{
  Mode mode = target->getMode();
  modeMap.emplace(mode , target);
  //MySerial.println("set");  //ここで送信するとメモリダンプする
  return true;
}

/*
* 現在のモードに応じた処理を行う
*/
Mode ModeManager::play()
{
  static bool wasFirst = true;
  static long startTime = 0;

  if (modeMap.empty()){
    return Mode::Error;
  }

  auto result = modeMap.find(this->modeNow);
  if (result == modeMap.end()){
    return Mode::Error;
  }

  ModeClass *mc = result->second;

  if (this->wasModeChanged){
    MySerial.println("change!");
    mc->whenChanged();
    startTime = millis();
    this->wasModeChanged = false;
    this->modeChangeProtect = false;
  }

  if (wasFirst){
    startTime = millis();
    wasFirst = false;
  }

  mc->execute(startTime,millis());

  return this->modeNow;
}

void ModeManager::changeMode(Mode next)
{
  MySerial.println((int)next);
  if (!(0 <= (int)next && next < Mode::Num)){
    //MySerial.println("err1");
    return;
  }

  if (this->modeChangeProtect){
    //MySerial.println("err2");
    return;
  }

  

  this->modePrev = this->modeNow;
  this->modeNow  = next;
  this->wasModeChanged = true;
}

void ModeManager::changeModeForce(Mode next)
{
  if (!(0 <= (int)next && next < Mode::Num)){
    return;
  }

  if (next == this->modeNow){
    return;
  }

  this->modePrev = this->modeNow;
  this->modeNow  = next;
  this->modeChangeProtect = true;
  this->wasModeChanged = true;
}