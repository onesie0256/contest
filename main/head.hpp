#pragma once
#include <map>
#include <math.h>
#include <queue>

#define MySerial Serial1


/**
* モードそれぞれを表す.モード1とか2とかの数字では表さない!
* ☆使い方
* Mode::なんとか
* ☆例
* Mode::Mode0_A
*/
enum class Mode
{
  /* 機体A(地上の探索)のモード */
  Mode0_A,  //Aの初期状態
  Forward,  //直進する
  Rotate,   //探索(回転)する
  Judge,    //見つけたコップが何かを判定する
  Obtain,   //見つけたコップへ向かう
  Return,   //ゴールに向かい,ゴール判定を行う
  Resume,   //ゴールして,forwardの状態に復帰する
  /* 機体B(山の上に行く)のモード */
  Mode0_B,  //Bの初期状態
  FindMount,//山を見つける.直進
  Climb,    //山を登り,相手側に回り込む
  Peak,     //頂上でコップを落とす
  Descent,  //山下り
  /* A,B共通 */
  // 12は旧CompassCalib。通信で使う後続モードの番号を維持する。
  BlackLine = 13,//黒い線を踏んだ時,フィールドに復帰する
  Avoid,    //自チーム・相手チームの機体を回避する
  Reset,    //リセットモードに遷移

  /* エラーが起きた時に */
  Error,
  /* モード数 */
  Num
};

/* 色 */
enum class Color
{
  White,
  Black,
  Red,
  Green,
  Blue,
  Cyan,
  Magenta,
  Yellow,
  UNKNOWN
};

enum class Side : uint8_t {
  North,
  South,
  Unknown
};

extern Side startSide;

/* _Gが付いてる変数の内容がprocessingに送られる! */
extern Mode mode_G;
extern uint8_t r_G;
extern uint8_t g_G;
extern uint8_t b_G;
extern uint8_t color_G;
extern uint8_t distance_G;
extern int16_t pitch_G;
extern int16_t roll_G;
extern int16_t yaw_G;
extern uint8_t isStacking_G;
extern uint16_t angle_G;

class ModeClass;
class ModeManager;
class A;
class B;
class Reset;
class FindMount;
class Climb;

Pushbutton button(ZUMO_BUTTON);
ZumoMotors motors;


/**
* モードを管理するマネージャークラス.林担当
*/
class ModeManager
{
private:
  std::map<Mode , ModeClass*> modeMap{};  //モードクラスの集合
  Mode modeNow = Mode::FindMount;           //現在のモード
  Mode modePrev = Mode::Mode0_A;          //1つ前のモード
  bool wasModeChanged = false;            //モードの変更に使用するフラグ
  bool modeChangeProtect = false;

public:
  ModeManager();
  bool set(ModeClass *target);
  Mode play();
  void changeMode(Mode next);
  Mode getModeNow() const;
  Mode getModePrev();
  void changeModeForce(Mode next);
};

ModeManager MM;

class ModeClass
{
protected:
  Mode myMode;

public:
  Mode getMode(){return myMode;}
  virtual void whenChanged()=0;
  virtual void execute(long startTime , long nowTime)=0;
};

class Timer
{
private:
  int flagStart = 0; // 0:待ち状態，1:現在計測中
  unsigned long startTime = 0;
public:
  int maintainState(unsigned long period);
  void reset();
  void start();
};

// class Buzzer
// {
// private:
// }
float readHeading();
float normalizeHeading(float heading);

