#include <ESP32Servo.h>
int quang = 34; // khai báo chân 34 với cảm biến quang
Servo myservo; // khai báo 
int pos=0; //góc quay của servo
void setup() {
  pinMode(quang,INPUT);
  myservo.attach(33);
}

void loop() {
  int giaTriQuang=digitalRead(quang); // đọc giá trị quang dưới dạng digital
  if(giaTriQuang==1){
    for(pos=0; pos<=180; pos+=1){
      myservo.write(pos);
      delay(15);
    }
  }
  else{
    for(pos = 180; pos>=0; pos-=1){
      myservo.write(pos);
      delay(15);
    }
  }
}
