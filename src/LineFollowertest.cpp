#include "MeMCore.h"

MeLineFollower lineFinder(PORT_1); /* Line Finder module  work with sensor 1*/


void setup()
{
  Serial.begin(9600);
}

void loop()
{
  int sensorState = lineFinder.readSensor2();

  Serial.println(sensorState);
  delay(200);
}
