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
 * Pro1 / Forward : 直進
 * 2秒間直進したらRotateへ移行
 * 15cm以内に障害物を検知したらJudgeへ移行
 ******************************************/

static const int FORWARD_SPEED = 200;
static const int FORWARD_TIME = 2000;
static const int FORWARD_DETECT_DISTANCE = 30;

class Pro1 : public ModeClass
{
public:
  Pro1(){
    this->myMode = Mode::Forward;
    MM.set(this);
  }

  void whenChanged() override{
    motors.setSpeeds(0, 0);
    MySerial.println("Forward Start");
  }

  void execute(long startTime, long nowTime) override{

    // 超音波センサで距離を測定
    int dist = readUltrasonicDistance();

    // 15cm以内に障害物を検知
    if (dist >= 0 && dist <= FORWARD_DETECT_DISTANCE){

      motors.setSpeeds(0, 0);

      MySerial.println("Object detected in Forward");

      MM.changeMode(Mode::Judge);
      return;
    }

    // 2秒経過したらRotateへ移行
    if (nowTime - startTime >= FORWARD_TIME){

      motors.setSpeeds(0, 0);

      MM.changeMode(Mode::Rotate);
      return;
    }

    // 直進
    motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
  }
};

/*******************************************
 * Pro2 / Judge : ロボットとコップの判定
 *
 * 1秒停止後、3秒間距離を測定
 * ロボット → Pro6
 * コップ   → Pro3
 ******************************************/

static const int JUDGE_WAIT_MS = 1000;
static const int JUDGE_MEASURE_MS = 3000;
static const int JUDGE_MOVE_THRESHOLD = 1;

class Pro2 : public ModeClass
{
private:
  bool measuring = false;
  int previousDist = -1;
  long measureStartTime = 0;

public:

  Pro2(){
    this->myMode = Mode::Judge;
    MM.set(this);
  }

  void whenChanged() override{

    measuring = false;
    previousDist = -1;

    motors.setSpeeds(0, 0);

    MySerial.println("pro2 Judge Start");
  }

  void execute(long startTime, long nowTime) override{

    motors.setSpeeds(0, 0);

    int dist = readUltrasonicDistance();

    // 最初の1秒間は停止
    if (!measuring){

      if (nowTime - startTime >= JUDGE_WAIT_MS){

        measuring = true;
        measureStartTime = nowTime;
        previousDist = dist;

        MySerial.println("Distance measurement start");
      }

      return;
    }

    // 距離の変化を確認
    if (dist >= 0 && previousDist >= 0){

      if (abs(dist - previousDist) >= JUDGE_MOVE_THRESHOLD){

        MySerial.println("Robot detected");

        // ロボットと判定したらPro6へ
        MM.changeMode(Mode::Rotate);

        return;
      }
    }

    if (dist >= 0){
      previousDist = dist;
    }

    // 3秒間変化がなければコップと判定
    if (nowTime - measureStartTime >= JUDGE_MEASURE_MS){

      MySerial.println("Cup detected");

      // コップと判定したらPro3へ
      MM.changeMode(Mode::Obtain);

      return;
    }
  }
};


/*******************************************
 * Pro3 / Obtain : コップを取りに行く
 *
 * 直進しながら距離を測定
 * 急激な距離変化を検知したら停止
 * 3秒間様子を見て元に戻ればObtain再開
 * 戻らなければRotateへ移行
 ******************************************/

static const int OBTAIN_SPEED = 150;
static const int OBTAIN_DISTANCE = 4;

// 距離の急激な変化を判断する閾値
static const int OBTAIN_CHANGE_THRESHOLD = 10;

// 停止して様子を見る時間
static const int OBTAIN_WAIT_MS = 3000;

// 元の距離に戻ったと判断する許容範囲
static const int OBTAIN_RECOVERY_RANGE = 5;

class Pro3 : public ModeClass
{
private:

  int previousDist = -1;

  // 異常な距離変化を確認中か
  bool waiting = false;

  // 急変する前の正常な距離
  int normalDist = -1;

  // 停止した時刻
  long waitStartTime = 0;

public:

  Pro3(){
    this->myMode = Mode::Obtain;
    MM.set(this);
  }

  void whenChanged() override{

    motors.setSpeeds(0, 0);

    previousDist = -1;
    normalDist = -1;
    waiting = false;

    MySerial.println("pro3 Obtain Start");
  }

  void execute(long startTime, long nowTime) override{

    int dist = readUltrasonicDistance();

    // 測定失敗時
    if (dist < 0){
      motors.setSpeeds(0, 0);
      return;
    }

    // 距離の異常変化を確認中
    if (waiting){

      motors.setSpeeds(0, 0);

      // 3秒経過するまで待つ
      if (nowTime - waitStartTime < OBTAIN_WAIT_MS){
        return;
      }

      // 元の正常な距離に戻ったか確認
      if (abs(dist - normalDist) <= OBTAIN_RECOVERY_RANGE){

        MySerial.println("Distance recovered");

        // Obtainを再開
        waiting = false;
        previousDist = dist;

        return;
      }

      // 元の距離に戻らなかった
      MySerial.println("Distance not recovered");

      motors.setSpeeds(0, 0);

      MM.changeMode(Mode::Rotate);
      return;
    }

    // 最初の正常な距離を記録
    if (previousDist < 0){

      previousDist = dist;
      normalDist = dist;
    }

    // 4cm以内に到達
    if (dist <= OBTAIN_DISTANCE){

      motors.setSpeeds(0, 0);

      MySerial.println("4cm reached");

      MM.changeMode(Mode::Return);
      return;
    }

    // 急激な距離変化を検知
    if (abs(dist - previousDist) >= OBTAIN_CHANGE_THRESHOLD){

      motors.setSpeeds(0, 0);

      // 変化する前の正常な距離を保存
      normalDist = previousDist;

      waiting = true;
      waitStartTime = nowTime;

      MySerial.println("Sudden distance change");

      return;
    }

    // 正常な距離を更新
    previousDist = dist;
    normalDist = dist;

    // 直進
    motors.setSpeeds(OBTAIN_SPEED, OBTAIN_SPEED);
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

/*******************************************
 * Pro6 / Rotate : 回転・障害物検知
 *
 * 1秒間回転
 * 15cm以内に障害物を検知したらJudgeへ
 * 障害物がなければForwardへ
 ******************************************/

static const int ROTATE_SPEED = 150;
static const int ROTATE_TIME = 1000;
static const int ROTATE_DETECT_DISTANCE = 30;

class Pro6 : public ModeClass
{
public:

  Pro6(){
    this->myMode = Mode::Rotate;
    MM.set(this);
  }

  void whenChanged() override{

    motors.setSpeeds(0, 0);

    MySerial.println("Rotate Start");
  }

  void execute(long startTime, long nowTime) override{

    // 超音波センサで距離を測定
    int dist = readUltrasonicDistance();

    // 15cm以内に障害物を検知
    if (dist >= 0 && dist <= ROTATE_DETECT_DISTANCE){

      motors.setSpeeds(0, 0);

      MySerial.println("Object detected in Rotate");

      MM.changeMode(Mode::Judge);
      return;
    }

    // 1秒経過したらForwardへ
    if (nowTime - startTime >= ROTATE_TIME){

      motors.setSpeeds(0, 0);

      MM.changeMode(Mode::Forward);
      return;
    }

    // その場で右回転
    motors.setSpeeds(ROTATE_SPEED, -ROTATE_SPEED);
  }
};

