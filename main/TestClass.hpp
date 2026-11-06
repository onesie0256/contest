#include "head.hpp"
#pragma once

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

    // 初期状態では停止し、loop()でユーザボタンによる開始を待つ。
    motors.setSpeeds(0, 0);
  }
};
 
/*******************************************
 *pro1 / Forward : 直進のみ
 *PWM=200
 *補足 特になし
 ******************************************/
static const int FORWARD_SPEED = 200;

class Pro1 : public ModeClass
{
public:
  Pro1(){
    this->myMode = Mode::Forward;
    MM.set(this);//マネージャークラスに自分のクラスを登録
  }

  void whenChanged() override{
    noTone(3);
    MySerial.println("pro1");
  }
  void execute(long startTime , long nowTime){
    motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
  }
};

static const int RETURN_TURN_HOME_SPEED = 200;  //自陣側へ向く回転の速度
static const int RETURN_TURN_HOME_MS    = 1000; //その時間 (今は約180°)
static const int RETURN_DRIVE_SPEED     = 120;  //直進の速度
static const int RETURN_TURN_SPEED      = 150;  //黒枠を避ける回転の速度
static const int RETURN_LEFT_TURN_MS    = 1100; //左回転の時間. 右回転はこの2倍

/*******************************************
 *pro4 / Return : コップを持って帰陣する
 *180°回転→直進→もし黒線を踏んだら自陣側へ回転
 *補足 pro5のBlackLineには遷移しないように書いている
 ******************************************/
class Pro4 : public ModeClass
{
private:
  /* コップを持った直後に自陣側へ向く. 今は180°回転するだけ */
  void turnToHome(){
    motors.setSpeeds(RETURN_TURN_HOME_SPEED, -RETURN_TURN_HOME_SPEED);
    delay(RETURN_TURN_HOME_MS);
    motors.setSpeeds(0, 0);
    delay(500);
  }

  /* 黒枠にぶつかった時: 左回転 → 少し進んで色を見直し → まだ黒なら2倍量の右回転 */
  void avoidBlackFrame(){
    motors.setSpeeds(-RETURN_TURN_SPEED, RETURN_TURN_SPEED);   //左回転
    delay(RETURN_LEFT_TURN_MS);
    motors.setSpeeds(RETURN_DRIVE_SPEED, RETURN_DRIVE_SPEED);
    delay(70);
    motors.setSpeeds(0, 0);
    delay(10);

    if (color_G == (uint8_t)Color::Black){
      motors.setSpeeds(RETURN_TURN_SPEED, -RETURN_TURN_SPEED); //2倍量の右回転
      delay(RETURN_LEFT_TURN_MS * 1.9);
      motors.setSpeeds(0, 0);
    }else{
      delay(200);
    }
  }

public:
  Pro4(){
    this->myMode = Mode::Return;
    MM.set(this);
  }

  void whenChanged() override{
    MySerial.println("pro4");
    turnToHome();
  }

  void execute(long startTime , long nowTime) override{
    Color c = (Color)color_G;

    if (c == Color::Red || c == Color::Blue){
      motors.setSpeeds(0, 0);       //ゴールライン到達 → 停止
      MM.changeMode(Mode::Resume);  //以降はResume担当
      return;
    }

    if (c == Color::Black){
      avoidBlackFrame();
      return;
    }

    motors.setSpeeds(RETURN_DRIVE_SPEED, RETURN_DRIVE_SPEED);
  }
};

/*******************************************
 * pro5 / BlackLine : 時間指定で後退・回転・前進し、元のモードへ戻る。
 * 各段階はdelayを使わず、経過時間で切り替える。
 ******************************************/
static const int BLACKLINE_SPEED = 200;
static const unsigned long BLACKLINE_BACK_MS = 100;
static const int BLACKLINE_TURN_MIN_MS = 750;
static const int BLACKLINE_TURN_MAX_MS = 1050;
static const unsigned long BLACKLINE_FORWARD_MS = 200;

class Pro5 : public ModeClass
{
private:
  unsigned long turnTime = 0;
  Mode resumeMode = Mode::Forward;

public:
  Pro5(){
    this->myMode = Mode::BlackLine;
    MM.set(this);
  }

  void whenChanged() override{
    MySerial.println("pro5");
    resumeMode = MM.getModePrev();
    turnTime = random(BLACKLINE_TURN_MIN_MS, BLACKLINE_TURN_MAX_MS);
  }

  void execute(long startTime, long nowTime) override{
    unsigned long time = (unsigned long)nowTime - (unsigned long)startTime;
    if (time < BLACKLINE_BACK_MS){
      motors.setSpeeds(-BLACKLINE_SPEED, -BLACKLINE_SPEED);
    }else if (time < BLACKLINE_BACK_MS + turnTime){
      motors.setSpeeds(BLACKLINE_SPEED, -BLACKLINE_SPEED);
    }else if (time < BLACKLINE_BACK_MS + turnTime + BLACKLINE_FORWARD_MS){
      motors.setSpeeds(BLACKLINE_SPEED, BLACKLINE_SPEED);
    }else{
      motors.setSpeeds(0, 0);
      MM.changeMode(resumeMode);
    }
  }
};
