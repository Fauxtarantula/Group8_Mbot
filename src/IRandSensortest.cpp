#include "MeMCore.h"
//#include "Motor_control.h"
int sensor = A0;
int IR = A1;
int value = 0;
MeBuzzer buzzer;
#define NOTE_A1 55;

void setup(){
  Serial.begin(9600);
  pinMode(sensor, INPUT);
  pinMode(IR, INPUT);
  
}

void loop(){
  Serial.println(analogRead(sensor));
  value = analogRead(sensor);
  delay(100);
  if(value > 650){
    Serial.println("overload");
    buzzer.tone(1109,1000);
  }
  Serial.println(analogRead(IR));
  delay(1000);
}
