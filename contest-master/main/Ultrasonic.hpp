#pragma once

const int trig = 2;
const int echo = 4;

inline int readUltrasonicDistance()
{
  unsigned long interval;

  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  interval = pulseIn(echo, HIGH, 23071);

  if (interval == 0) {
    return -1;
  }

  return (0.61 * 25 + 331.5) * interval / 10000 / 2;
}