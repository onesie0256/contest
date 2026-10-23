import processing.serial.*;
Serial port;
int mode;
int green, red, blue, green_p, red_p, blue_p;
boolean sof_f = false; // SoF(Start of flame) を発見したかどうかのフラグ
int time = 0;
final int timeOut = 5000;
final byte target = 37;
int packet = 0;
int recentAddress = 0;

int color_p = 0;
int distance_p = 0;
int pitch_p = 0;
int roll_p = 0;
int yaw_p = 0;
int isStacking_p = 0;


void setup() {
  size(1200, 400);
  background(255);
  //println(Serial.list());
  // String arduinoPort = Serial.list()[1];
  // port = new Serial(this, arduinoPort, 9600 );
  port = new Serial(this, "com8", 115200 ); // シリアルポート名は各自の環境に合わせて適宜指定
  red_p = 0;
  green_p = 0;
  blue_p = 0;
  
  send_request();
  time = millis();
}

void draw() {
  red_p = red;
  green_p = green;
  blue_p = blue;
  
  background(red_p,green_p,blue_p);
  stroke(0);
  fill(255);
  rect(0, height-200, 350, 200);
  textSize(50);
  fill(0);
  text("mode=", 20, height-10); text((int)mode, 200, height-10);
  String rgbText = "R:"+str(red_p)+"G:"+str(green_p)+"B:"+str(blue_p);
  text(rgbText, 20, height-60);
  noFill();
  textSize(20);
  text("color: "+str(color_p), 20, height-180);
  text("distance: "+str(distance_p)+" cm", 20, height-160);
  text("color: "+str(color_p), 20, height-180);
  text("pitch: "+str(pitch_p)+"°  roll: "+str(yaw_p)+"°  yaw: "+str(yaw_p)+"°", 20, height-140);
  if (isStacking_p == 1){
    fill(255,0,0);
    text("stacking!!!", 20, height-120);
    fill(0);
  }
  else {
    text("not stacking", 20, height-120);
  }
  
  
  if (millis() - time >= timeOut){
    send_request();
    println("リクエスト再送信");
    time = millis();
  }
}

void keyPressed() { // keyが押されると呼ばれる関数
  if (key == 's') {
    noLoop(); // draw()の繰返し処理を停止
  } else if (key == 'c') {
    background(255); // draw()の繰返し処理を再開
    loop();
  }
}


// 通信方式2
void serialEvent(Serial p) {

  int l = p.available(); // 受信バッファ内のデータ数
  boolean bod_f = false; // 1組のデータ(block of data)が得られたか？

  while (l>0) { // 受信バッファ内にデータがある場合
    if (sof_f == false) { // SoFを発見していない場合
      if (p.read() == 'H') { // SoF(Start of Frame)の検査
        sof_f = true; // SoFの発見
      }
      l--; // 受信バッファのデータ数の修正
    }
    if (sof_f == true) { // SoFを発見している場合
      if (l >= 15) { // 受信バッファのデータ数が15以上
        
        recentAddress = p.read();
        packet = p.read();
        mode =  p.read();
        red =  p.read();
        green = p.read();
        blue =  p.read();
        color_p = p.read();
        distance_p = p.read();
        pitch_p = read2byte(p);
        roll_p = read2byte(p);
        yaw_p = read2byte(p);
        isStacking_p = p.read();

        print(" RGB = ");
        println(red, green, blue);
        print(" color =");
        println(color_p);
        print(" distance =");
        println(distance_p);
        print(" attitude =");
        println(pitch_p,roll_p,yaw_p);
        print(" isStacking =");
        println(isStacking_p);
        

        bod_f = true; // 1組のデータを読み込んだ
        sof_f = false; // 1組のデータを読み取ったのでSoFの発見をクリア
        l -= 6; // 受信バッファのデータ数の修正
        time = millis();
      } else { // 受信バッファのデータ数不足の場合
        break; // whileループを中断
      }
    }
  }
  if (bod_f == true) // 1組のデータを読み込んだので
    send_request(); // 次のデータ送信要求を送信
}
// このプログラムではHに続くデータが6つ（1バイトのデータが6つ）で一組を想定している．
// プログラム中の6の部分は適宜変更が必要

void send_request(){
  port.write('H');
  port.write(target);
  port.write(1);
}

/**
* シリアルポートから2バイト読み取り,
* 16ビットの整数を返す
*/
int read2byte(Serial p){
  int high = p.read();  //上位8ビット受信
  int low = p.read();  //下位8ビット受信
  
  //2つのビット列を連結して返す
  return (high << 8 ) + low;
}
