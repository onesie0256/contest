/**
仮のコードです.
*/
#include "head.hpp"
#include "TestClass.hpp"

A a;
B b;

void setup()
{
  MySerial.begin(115200);
  while (!MySerial){
    delay(1);
  }
  a = A();
  b = B();
  MySerial.println("start");
}

void loop()
{
  MM.play();
  delay(50);
}