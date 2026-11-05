
#include <ZumoMotors.h>
#include <Pushbutton.h>

const int trig = 2;
const int echo = 4;

ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);

int mode = 0;  // 0:Search、1:Judge、2:Done、3:Obtain、4:次のモード

unsigned long judgeStartTime = 0;
unsigned long measureStartTime = 0;

bool measuring = false;
int previousDist = -1;

const int MOVE_THRESHOLD = 1;
const int OBTAIN_DISTANCE = 3;  // Obtain終了距離(cm)

int distance() {
  unsigned long interval;
  int dist;

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  interval = pulseIn(echo, HIGH, 23071);

  if (interval == 0) {
    return -1;
  }

  dist = (0.61 * 25 + 331.5) * interval / 10000 / 2;
  return dist;
}

void setup() {
  Serial1.begin(9600);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  motors.setSpeeds(0, 0);
  button.waitForButton();
}

void loop() {
  int dist = distance();

  Serial1.print("Mode: ");
  Serial1.print(mode);
  Serial1.print("  Distance: ");

  if (dist >= 0) {
    Serial1.print(dist);
    Serial1.println(" cm");
  } else {
    Serial1.println("測定失敗");
  }

  // Searchモード
  if (mode == 0) {
    motors.setSpeeds(150, 150);

    if (dist >= 0 && dist <= 15) {
      mode = 1;
      judgeStartTime = millis();
      measuring = false;
      previousDist = -1;

      Serial1.println("Judge mode start");
    }
  }

  // Judgeモード
  else if (mode == 1) {
    motors.setSpeeds(0, 0);

    if (!measuring) {
      if (millis() - judgeStartTime >= 1000) {
        measuring = true;
        measureStartTime = millis();
        previousDist = dist;

        Serial1.println("Distance measurement start");
      }
    }
    else {
      if (dist >= 0 && previousDist >= 0) {
        if (abs(dist - previousDist) >= MOVE_THRESHOLD) {
          Serial1.println("ロボット");
          mode = 2;
        }
      }

      if (mode == 1) {
        if (dist >= 0) {
          previousDist = dist;
        }

        if (millis() - measureStartTime >= 3000) {
          Serial1.println("コップ");

          mode = 3;  // Obtainモードへ移行

          Serial1.println("Obtain mode start");
        }
      }
    }
  }

  // Doneモード（ロボットと判定）
  else if (mode == 2) {
    motors.setSpeeds(0, 0);
  }

  // Obtainモード（コップを取りに行く）
  else if (mode == 3) {

    if (dist >= 0) {

      if (dist <= OBTAIN_DISTANCE) {
        motors.setSpeeds(0, 0);

        Serial1.println("3cm以内に到達");
        Serial1.println("Next mode");

        mode = 4;  // 次のモードへ移行
      }
      else {
        motors.setSpeeds(150, 150);  // 直進
      }

    }
    else {
      motors.setSpeeds(0, 0);  // 測定失敗時は安全のため停止
    }
  }

  // 次のモード
  else if (mode == 4) {
    motors.setSpeeds(0, 0);
  }

  delay(100);
}
