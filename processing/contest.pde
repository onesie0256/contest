import processing.serial.*;

// head.hpp の enum class Color と同じ順番にすること
final String[] colorToStr = {
  "white", "black", "red", "green", "blue", "cyan", "magenta", "yellow", "UNKNOWN"
};

// head.hpp の enum class Mode と同じ順番にすること
final String[] modeToStr = {
  "Mode0_A", "Forward", "Rotate", "Judge", "Obtain", "Return", "Resume",
  "Mode0_B", "FindMount", "Climb", "Peak", "Descent",
  "CompassCalib", "BlackLine", "Avoid", "Reset", "Error"
};

// 範囲外の値が来ても落ちないように文字列へ変換する
String lookup(String[] table, int index) {
  if (index < 0 || index >= table.length) {
    return "?(" + index + ")";
  }
  return table[index];
}

final int[] origin_x  = {0 , 600 , 0};
final int[] origin_y = {0 , 300 , 300};
final int _w = 1200;
final int _h = 600;

Serial port;
int[] mode = {0,0,0};
int green, red, blue;
int[] green_p = {0,0,0};
int[] red_p = {0,0,0};
int[] blue_p = {0,0,0};
boolean sof_f = false; // SoF(Start of flame) を発見したかどうかのフラグ
int time = 0;
final int timeOut = 1000;
final byte target = 37;
int packet = 0;
int recentAddress = 0;

int[] color_p = {0,0,0};
int[] distance_p = {0,0,0};
int[] pitch_p = {0,0,0};
int[] roll_p = {0,0,0};
int[] yaw_p = {0,0,0};
int[] isStacking_p = {0,0,0};

void setup() {
  size(1200, 600 , P3D);
  background(255);
  //println(Serial.list());
  // String arduinoPort = Serial.list()[1];
  // port = new Serial(this, arduinoPort, 9600 );
  port = new Serial(this, "com13", 9600 ); // シリアルポート名は各自の環境に合わせて適宜指定
  
  send_request();
  time = millis();
}

void draw() {
  background(255);
  stroke(0);
  fill(255);
  
  drawWindow(0 , 0 , 37);
  //drawWindow(600 , 0 , 38);
  //drawWindow(0 , 300 , 39);
  
  if (millis() - time >= timeOut){
    send_request();
    println("リクエスト再送信");
    time = millis();
  }
}

void drawWindow(int x , int y , int address)
{
  int idx = address - 37;
  int w = x + _w;
  int h = y + _h;
  noFill();
  rect(x, y, _w, _h);
  //rect(w-250, y, 250, 250);
  textSize(50);
  fill(0);
  text("Num.", x+20, y+40); text(str(address), x+150, y+40);
  text("mode: ", 20+x, h-10); text(lookup(modeToStr, mode[idx]), x+170, h-10);
  String rgbText = "R:"+str(red_p[idx])+"G:"+str(green_p[idx])+"B:"+str(blue_p[idx]);
  text(rgbText, 20+x, h-60);
  noFill();
  textSize(20);
  text("color: "+lookup(colorToStr, color_p[idx]), 20+x, h-180);
  text("distance: "+str(distance_p[idx])+" cm", x+20, h-160);
  //text("color: "+str(color_p), 20, height-180);
  text("pitch: "+str(pitch_p[idx])+"°  roll: "+str(roll_p[idx])+"°  yaw: "+str(yaw_p[idx])+"°", 20+x, h-140);
  if (isStacking_p[idx] == 1){
    fill(255,0,0);
    text("stacking!!!", 20+x, h-120);
    fill(0);
  }
  else {
    text("not stacking", 20+x, h-120);
  }
  fill(red_p[idx],green_p[idx],blue_p[idx]);
  rect(w-250-100, y, 100, 100);
  
  stroke(255);
  translate(600,300,-100);
  rotateX(radians(-30));
  rotateY(radians(135));
  
  drawAxis();
  //rotateX(radians((float)pitch_p[idx]));
  rotateY(- radians((float)yaw_p[idx]-270));
  fill(255);
  stroke(0);
  box(100,100,400);
  
  
  
}

void keyPressed() { // keyが押されると呼ばれる関数
  if (key == 's') {
    noLoop(); // draw()の繰返し処理を停止
  } else if (key == 'c') {
    background(255); // draw()の繰返し処理を再開
    loop();
  }
}

int receiveData(Serial p , int address)
{
  //if (p.available() < 13){
  //  return 0;
  //}
  
  int idx = address - 37;
 
  mode[idx] =  p.read();
  red_p[idx] =  p.read();
  green_p[idx] = p.read();
  blue_p[idx] =  p.read();
  color_p[idx] = p.read();
  distance_p[idx] = p.read();
  pitch_p[idx] = cnv(read2byte(p));
  roll_p[idx] = cnv(read2byte(p));
  yaw_p[idx] = cnv(read2byte(p));
  isStacking_p[idx] = p.read();

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
  return 13;
}



int receiveStr(Serial p)
{
  int l = p.available();
  int received = 0;
  
  //for (;l > 0; l--){
  //  ;
  //}
  return 0;
}

// 通信方式2
final int basicLen = 2; // Hの後ろのヘッダ(アドレス, パケット種別)のバイト数
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
      if (l >= 15) { // 受信バッファのデータ数がヘッダ長以上
        recentAddress = p.read();
        packet = p.read();
        println(l);
        println(recentAddress);
        receiveData(p,recentAddress);
        
        //switch (packet){
        //  case 1:
        //  l -= receiveData(p);
        //  break;
        //  default:
        //  break;
        //}

        bod_f = true; // 1組のデータを読み込んだ
        sof_f = false; // 1組のデータを読み取ったのでSoFの発見をクリア
        l -= 15; // 受信バッファのデータ数の修正
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

int cnv(int x)
{
  if (x >= 32768){
    return x-65535; 
  }
  return x;
}

void drawAxis()
{
  //X軸
  stroke(255,0,0);
  beginShape();
  vertex(0,0,0);
  vertex(-300,0, 0);
  vertex(-300,0, 0);
  vertex(0,0, 0);
  endShape();
  //X軸の文字[E]
  stroke(255,0,0);
  beginShape();
  vertex(-310,0,0);
  vertex(-330,0, 0);
  vertex(-330,0, 0);
  vertex(-310,0, 0);
  endShape();
  stroke(255,0,0);
  beginShape();
  vertex(-310,0,0);
  vertex(-310,0, -10);
  vertex(-310,0, -10);
  vertex(-310,0, 0);
  endShape();
  stroke(255,0,0);
  beginShape();
  vertex(-320,0,0);
  vertex(-320,0, -10);
  vertex(-320,0, -10);
  vertex(-320,0, 0);
  endShape();
  stroke(255,0,0);
  beginShape();
  vertex(-330,0,0);
  vertex(-330,0, -10);
  vertex(-330,0, -10);
  vertex(-330,0, 0);
  endShape();
  //Y軸
  stroke(0,255,0);
  beginShape();
  vertex(0,0,0);
  vertex(0,-300, 0);
  vertex(0,-300, 0);
  vertex(0,0, 0);
  endShape();
  
  //Z軸
  stroke(0,0,255);
  beginShape();
  vertex(0,0,0);
  vertex(0,0, 300);
  vertex(0,0, 300);
  vertex(0,0, 0);
  endShape();
  //Z軸の文字[N]
  beginShape();
  vertex(-10,0,310);
  vertex(-10,0, 330);
  vertex(-10,0, 330);
  vertex(-10,0, 310);
  endShape();
  beginShape();
  vertex(10,0,310);
  vertex(10,0, 330);
  vertex(10,0, 330);
  vertex(10,0, 310);
  endShape();
  beginShape();
  vertex(-10,0,310);
  vertex(10,0, 330);
  vertex(10,0, 330);
  vertex(-10,0, 310);
  endShape();
}
