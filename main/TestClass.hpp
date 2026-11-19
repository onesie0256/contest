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

    // if (nowTime - startTime >= 5000){
    //   //noTone(3);
    //   MM.changeMode(Mode::Forward);
    // }
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

  
  void execute(long startTime, long nowTime) override {

    // 超音波センサで距離を測定
    int dist = readUltrasonicDistance();

    // Processingに距離を送る
    if (dist < 0) {
      distance_G = 255;
    } else {
      distance_G = constrain(dist, 0, 255);
    }

    // 30cm以内に障害物を検知したらJudgeへ
    if (dist >= 0 && dist <= FORWARD_DETECT_DISTANCE) {
      motors.setSpeeds(0, 0);
      MM.changeMode(Mode::Judge);
      return;
    }

    // 2秒経過したらRotateへ
    if (nowTime - startTime >= FORWARD_TIME) {
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
static const int OBTAIN_DISTANCE = 6;

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


/*******************************************
 * Pro4 / Return : ゴールへ帰還
 ******************************************/

static const int RETURN_TURN_HOME_SPEED = 200;
static const int RETURN_TURN_HOME_MS    = 1000;

static const int RETURN_DRIVE_SPEED     = 120;
static const int RETURN_TURN_SPEED      = 150;
static const int RETURN_LEFT_TURN_MS    = 1100;

class Pro4 : public ModeClass
{
private:

  // Obtainから移行したときに自陣方向を向く
  void turnToHome(){

    motors.setSpeeds(RETURN_TURN_HOME_SPEED, -RETURN_TURN_HOME_SPEED);
    delay(RETURN_TURN_HOME_MS);

    motors.setSpeeds(0, 0);
    delay(500);
  }

  // 黒線を回避する
  void avoidBlackFrame(){

    // 左回転
    motors.setSpeeds(-RETURN_TURN_SPEED, RETURN_TURN_SPEED);
    delay(RETURN_LEFT_TURN_MS);

    // 少し前進
    motors.setSpeeds(RETURN_DRIVE_SPEED, RETURN_DRIVE_SPEED);
    delay(70);

    motors.setSpeeds(0, 0);
    delay(100);

    // ★ 回避後にカラーセンサを読み直す
    getColor();
    color_G = (uint8_t)Nearest_Neighbor();

    // まだ黒線上なら反対方向へ大きく回転
    if (color_G == (uint8_t)Color::Black){

      motors.setSpeeds(RETURN_TURN_SPEED, -RETURN_TURN_SPEED);
      delay(RETURN_LEFT_TURN_MS * 1.9);

      motors.setSpeeds(0, 0);
      delay(200);
    }
  }

public:

  Pro4(){
    this->myMode = Mode::Return;
    MM.set(this);
  }

  void whenChanged() override{

    motors.setSpeeds(0, 0);

    MySerial.println("Return Start");

    turnToHome();
  }

  void execute(long startTime, long nowTime) override{

    Color c = (Color)color_G;

    // 赤または青を踏んだらゴール
    if (c == Color::Red || c == Color::Blue){

      motors.setSpeeds(0, 0);

      MySerial.println("Goal detected");

      MM.changeMode(Mode::Resume);
      return;
    }

    // 黒線を踏んだら回避
    if (c == Color::Black){

      motors.setSpeeds(0, 0);

      avoidBlackFrame();
      return;
    }

    // それ以外は直進
    motors.setSpeeds(RETURN_DRIVE_SPEED, RETURN_DRIVE_SPEED);
  }
};

 /*******************************************
 *pro5 / BlackLine : 黒線を踏んだ時フィールドに復帰
 *コンパイルエラーにならないようmain.inoにcheckBlackLine()で判定させている
 *補足 統合した方がコードが簡略化できる可能性、ただし条件分岐が多いのでBlackLine_a,bを作るのも検討すべき 
 ******************************************/

static const int BLACKLINE_SPEED       = 200;
static const int BLACKLINE_BACK_MS     = 100;   //後退
static const int BLACKLINE_TURN_MIN_MS = 750;   //回転(ランダムな角度)
static const int BLACKLINE_TURN_MAX_MS = 1050;
static const int BLACKLINE_FORWARD_MS  = 200;   //復帰のために少し前進

class Pro5 : public ModeClass
{
public:
  Pro5(){
    this->myMode = Mode::BlackLine;
    MM.set(this);
  }

  void whenChanged() override{
    MySerial.println("pro5");
  }

  void execute(long startTime , long nowTime) override{
    motors.setSpeeds(-BLACKLINE_SPEED, -BLACKLINE_SPEED);
    delay(BLACKLINE_BACK_MS);

    motors.setSpeeds(BLACKLINE_SPEED, -BLACKLINE_SPEED);
    delay(random(BLACKLINE_TURN_MIN_MS, BLACKLINE_TURN_MAX_MS));

    motors.setSpeeds(BLACKLINE_SPEED, BLACKLINE_SPEED);
    delay(BLACKLINE_FORWARD_MS);

    MM.changeMode(MM.getModePrev());  //フィールドに復帰 = 黒線を踏む前のモードに戻る
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
  private:
  // Judgeから移行した場合、Rotate中の障害物判定を無効にする
  bool ignoreObstacle = false;
public:

  Pro6(){
    this->myMode = Mode::Rotate;
    MM.set(this);
  }

 void whenChanged() override{

    motors.setSpeeds(0, 0);

    // 直前のモードがJudgeなら障害物判定を無効にする
    ignoreObstacle = (MM.getModePrev() == Mode::Judge);

    MySerial.println("Rotate Start");
}
 
  void execute(long startTime, long nowTime) override {

    // 超音波センサで距離を測定
    int dist = readUltrasonicDistance();

    // Processingに距離を送る
    if (dist < 0) {
      distance_G = 255;
    } else {
      distance_G = constrain(dist, 0, 255);
    }

    // Judgeから移行した場合は障害物判定をしない
    if (!ignoreObstacle) {

      // 30cm以内に障害物を検知したらJudgeへ
      if (dist >= 0 && dist <= ROTATE_DETECT_DISTANCE) {
        motors.setSpeeds(0, 0);
        MM.changeMode(Mode::Judge);
        return;
      }
    }

    // 1秒経過したらForwardへ
    if (nowTime - startTime >= ROTATE_TIME) {
      motors.setSpeeds(0, 0);
      MM.changeMode(Mode::Forward);
      return;
    }

    // その場で右回転
    motors.setSpeeds(ROTATE_SPEED, -ROTATE_SPEED);
}

};

/*******************************************
 * Resume : ゴール後にフィールドへ復帰
 ******************************************/

static const int RESUME_BACK_SPEED = 150;
static const int RESUME_BACK_TIME  = 300;

static const int RESUME_TURN_SPEED = 200;
static const int RESUME_TURN_TIME  = 1000;

class Pro7 : public ModeClass
{
public:

  Pro7(){
    this->myMode = Mode::Resume;
    MM.set(this);
  }

  void whenChanged() override{

    motors.setSpeeds(0, 0);

    MySerial.println("Resume Start");
  }

  void execute(long startTime, long nowTime) override{

    // 少し後退
    motors.setSpeeds(-RESUME_BACK_SPEED, -RESUME_BACK_SPEED);
    delay(RESUME_BACK_TIME);

    motors.setSpeeds(0, 0);
    delay(200);

    // 180度回転
    motors.setSpeeds(RESUME_TURN_SPEED, -RESUME_TURN_SPEED);
    delay(RESUME_TURN_TIME);

    motors.setSpeeds(0, 0);
    delay(300);

    // Forwardへ復帰
    MM.changeMode(Mode::Forward);
  }
};

/* ================================================================
 * pro11 / CompassCalib : スタート位置での向きから、自ゴールがどちら側かを判定
 * スタート位置に置いてボタンを押す
 *   → 0.1sごとに3回、向きを取る
 *   → 同じ側(北 or 南)が2回以上 : 記憶して成功を出力し、Forwardへ進む
 *   → それ以外(東西など)       : このモードのまま成功するまでループ
 *補足 最適化されてない、ほんとはここにあるべきコードじゃないはず
 * ================================================================ */

static const float COMPASS_NS_TOLERANCE_DEG = 20.0;  //南北とみなす許容角(±)
static const int   COMPASS_SAMPLES          = 3;     //向きを取る回数
static const int   COMPASS_SAMPLE_MS        = 100;   //その間隔
static const int   COMPASS_NEED_SAME        = 2;     //同じ側がこの回数以上なら採用

class Pro11 : public ModeClass
{
private:
  /* 1回分の向き(0〜360°)を 北/南/それ以外 に分類する */
  Side classify(float heading){
    if (heading <= COMPASS_NS_TOLERANCE_DEG || heading >= 360.0 - COMPASS_NS_TOLERANCE_DEG){
      return Side::North;
    }
    if (heading >= 180.0 - COMPASS_NS_TOLERANCE_DEG && heading <= 180.0 + COMPASS_NS_TOLERANCE_DEG){
      return Side::South;
    }
    return Side::Unknown;
  }

  /* 0.1sごとに3回向きを取って判定する. 判定できなければUnknown */
  Side judge(){
    int north = 0;
    int south = 0;

    for (int i = 0; i < COMPASS_SAMPLES; i++){
      float heading = readHeading();
      Side side = classify(heading);
      MySerial.println("pro11");

      if (side == Side::North) north++;
      if (side == Side::South) south++;

      if (i < COMPASS_SAMPLES - 1){
        delay(COMPASS_SAMPLE_MS);
      }
    }

    if (north >= COMPASS_NEED_SAME) return Side::North;
    if (south >= COMPASS_NEED_SAME) return Side::South;
    return Side::Unknown;
  }

public:
  Pro11(){
    this->myMode = Mode::CompassCalib;
    MM.set(this);
  }

  void whenChanged() override{
    MySerial.println(F("スタート位置にロボットを置き、ボタンを押してください"));
  }

  void execute(long startTime , long nowTime) override{
    if (!button.getSingleDebouncedPress()){
      return;
    }

    Side side = judge();

    if (side == Side::Unknown){
      MySerial.println(F("東西向き(または判定不能)でした。向きを直して、もう一度ボタンを押してください"));
      return;
    }

    //startSide = side;
    if (side == Side::North){
      MySerial.println(F("成功: スタート位置での向きを記憶しました (北)"));
    }else{
      MySerial.println(F("成功: スタート位置での向きを記憶しました (南)"));
    }

    MM.changeMode(Mode::Forward);
  }
};
