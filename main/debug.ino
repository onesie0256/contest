
#include "head.hpp"

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

  debugMessage(msg.c_str());
}
