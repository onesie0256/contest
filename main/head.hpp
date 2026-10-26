#pragma once
#include <map>

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
  CompassCalib,//初めにキャリブレーションをする
  BlackLine,//黒い線を踏んだ時,フィールドに復帰する
  Avoid,    //自チーム・相手チームの機体を回避する
  /* エラーが起きた時に */
  Error,
  /* モード数 */
  Num
};

enum class FieldColor : uint8_t {
  White,
  Black,
  Red,
  Blue,
  Other
};

enum class Side : uint8_t {
  North,
  South,
  Unknown
};

extern ZumoMotors motors;
extern Pushbutton button;
extern Side startSide;

FieldColor measureColor();
float readHeading();

class ModeClass;
class ModeManager;
class A;
class B;

/**
* モードを管理するマネージャークラス.林担当
*/
class ModeManager
{
private:
  std::map<Mode , ModeClass*> modeMap{};  //モードクラスの集合
  Mode modeNow = Mode::Mode0_A;           //現在のモード
  Mode modePrev = Mode::Mode0_A;          //1つ前のモード
  bool wasModeChanged = false;            //モードの変更に使用するフラグ
  bool modeChangeProtect = false;

public:
  ModeManager();
  bool set(ModeClass *target);
  Mode play();
  void changeMode(Mode next);
  Mode getModePrev();
  void changeModeForce(Mode next);
};

class ModeClass
{
protected:
  Mode myMode;

public:
  Mode getMode(){return myMode;}
  virtual void whenChanged()=0;
  virtual void execute(long startTime , long nowTime)=0;
};

ModeManager MM;