#include "MeMCore.h"
//#include "Motor_control.h"
int sensor = A2;
int IRsensor = A3;
int sensorvalue = 0;
int speed1 = 150;
int speed2 = -150;
int load,line,IR,ultra = 0; // 1. load sensor val, line finder sensor

MeBuzzer buzzer;
MeDCMotor motor1(M1);
MeDCMotor motor2(M2);
MeLineFollower lineFinder(PORT_1);
MeUltrasonicSensor ultraSensor(PORT_4);

void turn(bool dir){//90 degree turn left ==1 , right == 0
  // motor1.stop();
  // motor2.stop();
  // motor1.run(-speed1);
  // motor2.run(-speed2);
  // delay(500);
  motor1.stop();
  motor2.stop();
  if(dir == 0){ //right
    motor1.run(-255);
    motor2.run(-255);
    delay(300);
    motor1.stop();
    motor2.stop();
    delay(300);
  }
  if(dir == 1){ //left
    motor1.run(255);
    motor2.run(255);
    delay(300);
    motor1.stop();
    motor2.stop();
    delay(300);
  }
}
void setup(){
  Serial.begin(9600);
  pinMode(sensor, INPUT);
  pinMode(IRsensor, INPUT);
}
void loop(){
  //Serial.println(analogRead(sensor));
  load = analogRead(sensor);
  line = lineFinder.readSensors();
  IR = analogRead(IRsensor);
  ultra = ultraSensor.distanceCm();

  if(load > 650){
    buzzer.tone(1109,50);
    motor1.stop();
    motor2.stop();
  }
  if(line != 3){ //turn if see black line value2 ==0
    if(line == 1){
      turn(0);
    }
    else{
      turn(1);
    }
  }
  if(IR < 300){ 
    while(TRUE){
      motor1.stop();
      motor2.stop();
    }
  }
  if(ultra < 10){
    turn(0);
  }
  Serial.println(IR);
  motor1.run(speed1);
  motor2.run(speed2);
}
