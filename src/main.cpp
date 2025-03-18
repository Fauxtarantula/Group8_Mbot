#include "MeMCore.h"
//#include "Motor_control.h"
MeDCMotor motor1(M1);
MeDCMotor motor2(M2);

void setup(){
  
  
}

void loop(){
  motor1.run(255);
  motor2.run(255);
  delay(1000);
  motor1.stop();
  motor2.stop();
  delay(100);
  motor1.run(-255);
  motor2.run(-255);
  delay(1000);
}
