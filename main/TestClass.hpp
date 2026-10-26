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
    MySerial.println("A");
  }

  void execute(long startTime , long nowTime){
    if (isFirst){
      //tone(3,this->f);
      MySerial.println("tone");
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

    if (measureColor() == FieldColor::Black){
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
    FieldColor c = measureColor();

    if (c == FieldColor::Red || c == FieldColor::Blue){
      motors.setSpeeds(0, 0);       //ゴールライン到達 → 停止
      MM.changeMode(Mode::Resume);  //以降はResume担当
      return;
    }

    if (c == FieldColor::Black){
      avoidBlackFrame();
      return;
    }

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

    startSide = side;
    if (side == Side::North){
      MySerial.println(F("成功: スタート位置での向きを記憶しました (北)"));
    }else{
      MySerial.println(F("成功: スタート位置での向きを記憶しました (南)"));
    }

    MM.changeMode(Mode::Forward);
  }
};
