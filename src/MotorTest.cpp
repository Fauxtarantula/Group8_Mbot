
#include "MeOrion.h"

// The two motor objects
MeDCMotor motor1(M1);
MeDCMotor motor2(M2);

int motorSpeed = 100;
int motorSpeed2 = -100; //mental note : use int instead -ve forward

void setup()
{
}

void loop()
{
  // Run both motors forwards at the same speed
  motor1.run(motorSpeed);
  motor2.run(motorSpeed2);
}
