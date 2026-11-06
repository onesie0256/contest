
#include "head.hpp"

// false: XBee受信側のシリアルモニタへ、そのまま読める文字列を送る。
// true : Processing向けの既存パケット形式（sendString）で送る。
// 受信側のシリアルモニタはXBeeの通信速度に合わせる（現在9600bps）。
static const bool DEBUG_USE_PROCESSING_PACKETS = false;

// デバッグ項目を追加するときは debugMessage("確認したい内容"); を使う。
// 数値を含む場合の例:
// String message = "distance=" + String(distance_G);
// debugMessage(message.c_str());
// 既存のSerial1/MySerial出力は各ファイルに残しているため、必要に応じて整理する。

// モード番号を文字列へ変換
const char* debugModeName(Mode mode)
{
  switch (mode) {
    case Mode::Mode0_A:     return "Mode0_A";
    case Mode::Forward:     return "Forward";
    case Mode::Rotate:      return "Rotate";
    case Mode::Judge:       return "Judge";
    case Mode::Obtain:      return "Obtain";
    case Mode::Return:      return "Return";
    case Mode::Resume:      return "Resume";
    case Mode::Mode0_B:     return "Mode0_B";
    case Mode::FindMount:   return "FindMount";
    case Mode::Climb:       return "Climb";
    case Mode::Peak:        return "Peak";
    case Mode::Descent:     return "Descent";
    case Mode::BlackLine:   return "BlackLine";
    case Mode::Avoid:       return "Avoid";
    case Mode::Reset:       return "Reset";
    case Mode::Error:       return "Error";
    default:                return "Unknown";
  }
}

// 色番号を文字列へ変換
const char* debugColorName(Color color)
{
  switch (color) {
    case Color::White:   return "White";
    case Color::Black:   return "Black";
    case Color::Red:     return "Red";
    case Color::Green:   return "Green";
    case Color::Blue:    return "Blue";
    case Color::Cyan:    return "Cyan";
    case Color::Magenta: return "Magenta";
    case Color::Yellow:  return "Yellow";
    default:             return "Unknown";
  }
}

// XBee通信で文字列を送る
void debugMessage(const char* message)
{
  // XBee経由でPCへ送信する。USBのSerialへの出力は行わない。
  // シリアルモニタではパケットのヘッダを付けず、1メッセージ1行で表示する。
  if (!DEBUG_USE_PROCESSING_PACKETS) {
    MySerial.println(message);
    return;
  }

  String text = String(message);
  text += '\n';
  sendString(text);
}

// 現在の状態を送る
void debugStatus()
{
  static unsigned long lastTime = 0;
  static Mode lastMode = Mode::Num;

  unsigned long now = millis();

  // モードが変更されたら通知
  if (mode_G != lastMode) {
    String msg = "MODE: ";
    msg += debugModeName(lastMode);
    msg += " -> ";
    msg += debugModeName(mode_G);

    debugMessage(msg.c_str());
    lastMode = mode_G;
  }

  // 1000msごとに状態を送信
  if (now - lastTime < 1000) {
    return;
  }

  lastTime = now;

  String msg;
  msg.reserve(120);

  msg += "STATUS t=";
  msg += now;

  msg += " mode=";
  msg += debugModeName(mode_G);

  msg += " color=";
  msg += debugColorName((Color)color_G);

  msg += " RGB=";
  msg += r_G;
  msg += ",";
  msg += g_G;
  msg += ",";
  msg += b_G;

  msg += " heading=";
  msg += String(readHeading(), 1);

  // mode_Gと管理中のモードの食い違い（未登録モードなど）を確認する。
  msg += " managerMode=";
  msg += debugModeName(MM.getModeNow());

  // 校正で記憶した自陣側を確認する。不要ならこのブロックをコメントアウトする。
  msg += " startSide=";
  switch (startSide) {
    case Side::North: msg += "North"; break;
    case Side::South: msg += "South"; break;
    default:          msg += "Unknown"; break;
  }

  debugMessage(msg.c_str());
}
