#include "head.hpp"

int Timer::maintainState(unsigned long period)
{
  unsigned long timeNow = millis();
  if (flagStart == 0) 
  {
    startTime = timeNow; // 計測を開始した時刻の値を覚えておく
    flagStart = 1; // 現在計測中にしておく
  }

  if (timeNow - startTime > period) 
  { 
    // 計測開始からの経過時間が指定時間を越えた
    flagStart = 0; // 待ち状態に戻しておく
    startTime = 0; // なくても良いが，形式的に初期化
    return 1;
  }
  else
    return 0;
}

void Timer::reset()
{
  flagStart = 0; // 待ち状態に戻しておく
  startTime = 0; // なくても良いが，形式的に初期化
}

void Timer::start()
{
  this->reset();
  startTime = millis(); // 計測を開始した時刻の値を覚えておく
  flagStart = 1; // 現在計測中にしておく
}

