#include "MeMCore.h"
#include "SoftwareSerial.h"

// pins
const int loadSensor = A2;
const int IRSensor = A3;

// constants
const int speed1 = 150;
const int speed2 = -150;
const int loadThresh = 650;
const int IRThresh = 300;
const int ultraThresh = 10;

int remoteSpeed1 = 100;
int remoteSpeed2 = -100;

// sensors
int load, line, IR, ultra = 0;

int turnOffset = 0;
int stop = 0; // stop moving when see IR array
int manual = 0; // mode
int ret = 0; // return to original
int hunt = 0; // hunt for IR
int decode = 0;
int remoteStop = 0; // stop by remote
int delayDur = 1000;
int firstManual, firstAuto = 1;

int debug  = 0;

MeIR ir;
MeBuzzer buzzer;
MeDCMotor motor1(M1);
MeDCMotor motor2(M2);
MeLineFollower lineFinder(PORT_1);
MeUltrasonicSensor ultraSensor(PORT_4);

void turn(bool dir) {//90 degree turn left ==1 , right == 0
  int turnDur = 310;
  int turnSpeed = 255;

  // motor1.stop(); // reverse
  // motor2.stop();
  // motor1.run(-speed1);
  // motor2.run(-speed2);
  // delay(500);

  motor1.stop();
  motor2.stop();
  if (dir == 0) { // right
    motor1.run(-turnSpeed);
    motor2.run(-turnSpeed);
    delay(turnDur);
    motor1.stop();
    motor2.stop();
    delay(turnDur);
    turnOffset += 1;
  }

  if (dir == 1) { // left
    motor1.run(turnSpeed);
    motor2.run(turnSpeed);
    delay(turnDur);
    motor1.stop();
    motor2.stop();
    delay(turnDur);
    turnOffset -= 1;
  }
}

void checkMode(int decode) {
  if (decode) { // switch modes
    uint32_t value = ir.value;
    Serial.println(value);

    switch (value >> 16 & 0xff) {
      case IR_BUTTON_B: // switch to manual
        if (debug) Serial.println("SWITCH TO MANUAL");
        manual = 1;
        break;
      case IR_BUTTON_C: // switch to auto
        if (debug) Serial.println("SWITCH TO AUTO");
        manual = 0;
        break;
    }
  }
}

void checkStop(int decode) {
  if (decode) {
    uint32_t value = ir.value;

    switch (value >> 16 & 0xff) {
      case IR_BUTTON_SETTING: // restart program
        if (debug) Serial.println("REMOTE STOP");
        remoteStop = 1;
        break;
    }
  }
}

void checkRestart(int decode) { // combined for the stop, return and hunt
  if (ir.decode()) {
    uint32_t value = ir.value;

    switch (value >> 16 & 0xff) {
      case IR_BUTTON_A: // restart program
        if (debug) Serial.println("RESTART");
        stop = 0;
        remoteStop = 0;
        ret = 0;
        hunt = 0;
        break;
    }
  }
}

void checkReturn(int decode) {
  // Serial.println("ee");
  if (decode) {
    uint32_t value2 = ir.value;

    switch (value2 >> 16 & 0xff) {
      case IR_BUTTON_E: // restart program
        if (debug) Serial.println("RETURN TO ORIGINAL SPOT");
        ret = 1;
        break;
    }
  }
}

void checkHunt(int decode) {
  if (decode) {
    uint32_t value2 = ir.value;

    switch (value2 >> 16 & 0xff) {
      case IR_BUTTON_D: // restart program
        if (debug) Serial.println("HUNT FOR IR");
        hunt = 1;
        break;
    }
  }
}

void turnReset() {
  turnOffset -= 2; // so we turn to face the back instead
  turnOffset %= 4; // wraparound
  if (turnOffset == 3) turnOffset = -1;
  else if (turnOffset == -3) turnOffset = 1;

  while (turnOffset != 0) {
    if (turnOffset > 0) { // facing right
      turn(1);
    }
    else { // facing left
      turn(0);
    }
  }
  motor1.stop();
  motor2.stop();
  delay(500);
}

// void moveReset() {
//   if (line == 3) { // move forward until one or both sensors see the line
//     motor1.run(speed1);
//     motor2.run(speed2);
//   }
//   else if (line != 0) { // only one sensor seeing line
//     motor1.run(-50);
//     motor1.run(-50);
//   }
  
//   motor1.stop();
//   motor2.stop();
//   delay(1000);
// }

void setup() {
  Serial.begin(9600);
  ir.begin();
  pinMode(loadSensor, INPUT);
  pinMode(IRSensor, INPUT);
}

void loop() {
  load = analogRead(loadSensor);
  line = lineFinder.readSensors(); // 3 is both white, 2 is right black, 1 is left black, 0 is both black
  IR = analogRead(IRSensor);
  ultra = ultraSensor.distanceCm();

  checkMode(decode);
  decode = ir.decode();

  if (!manual) {
    firstManual = 1;
    if (firstAuto) {
      turnOffset = 0;
      motor1.stop();
      motor2.stop();
      buzzer.tone(500, 1000);
      firstAuto = 0;
    }
    if (debug) Serial.println("AUTO");

    if (load > loadThresh) { // sound buzzer if dust bag is full
      buzzer.tone(1109, 50);
      motor1.stop();
      motor2.stop();
    }

    if (line != 3) {
      if (line == 2) turn(1); // turn left if right sensor sees black
      else turn(0); // turn right if left or both sensors see black
    }

    checkStop(decode);
    // Serial.println(stop);
    if ((IR < IRThresh) | remoteStop) { // if transmitter sees IR array stop forever 
      stop = 1;
      while (stop == 1) {
        motor1.stop();
        motor2.stop();
        checkRestart(decode);
      }
    }

    if (ultra < ultraThresh) turn(0); // turn if see object

    motor1.run(speed1);
    motor2.run(speed2);

    checkReturn(decode);
    if (ret) {
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      turnReset();

      while (line == 3) { // move forward until see black line
        line = lineFinder.readSensors();
        motor1.run(100);
        motor2.run(-100);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);
      while (line != 0) {  // turn to be straight
        line = lineFinder.readSensors();
        motor1.run(-100);
        motor2.run(-100);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      turn(1); // turn right to align in the other axis
      delay(delayDur);
      // buzzer.tone(1109, 2000);
      line = lineFinder.readSensors();
      while (line == 3) { // move forward until see black line
        line = lineFinder.readSensors();
        motor1.run(100);
        motor2.run(-100);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);
      while (line != 0) {  // turn to be straight
        line = lineFinder.readSensors();
        motor1.run(-100);
        motor2.run(-100);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      turn(1); // turn to face the front
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      while (ret) checkRestart(decode);
    }

    checkHunt(decode);
    if (hunt) {
      while (IR > IRThresh) {
        motor1.run(-100);
        motor2.run(-100);
        IR = analogRead(IRSensor);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      while (line == 3) { // move forward until see black line
        line = lineFinder.readSensors();
        motor1.run(-100);
        motor2.run(100);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      while (hunt) checkRestart(decode);
    }
  }
  else {
    if (debug) Serial.print("MANUAL ");
    firstAuto = 1;
    if (firstManual) {
      motor1.stop();
      motor2.stop();
      buzzer.tone(1500, 1000);
      firstManual = 0;
    }

    if (ir.decode()) { // remote control
      uint32_t value = ir.value;
  
      switch (value >> 16 & 0xff) {
        case IR_BUTTON_SETTING:
          motor1.stop();
          motor2.stop();
          if (debug) Serial.println("STOP");
          break;
        case IR_BUTTON_PLUS:
          motor1.run(remoteSpeed1);
          motor2.run(remoteSpeed2);
          if (debug) Serial.println("FORWARD");
          break;
        case IR_BUTTON_MINUS:
          motor1.run(-remoteSpeed1);
          motor2.run(-remoteSpeed2);
          if (debug) Serial.println("BACKWARD");
          break;
        case IR_BUTTON_NEXT:
          motor1.run(-remoteSpeed1);
          motor2.run(remoteSpeed2);
          if (debug) Serial.println("TURN RIGHT");
          break;
        case IR_BUTTON_PREVIOUS:
          motor1.run(remoteSpeed1);
          motor2.run(-remoteSpeed2);
          if (debug) Serial.println("PREV");
          break;
        case IR_BUTTON_1:
          remoteSpeed1 = 100;
          remoteSpeed2 = -100;
          if (debug) Serial.println("SPEED 1");
          break;
        case IR_BUTTON_2:
          remoteSpeed1 = 150;
          remoteSpeed2 = -150;
          if (debug) Serial.println("SPEED 2");
          break;
        case IR_BUTTON_3:
          remoteSpeed1 = 200;
          remoteSpeed2 = -200;
          if (debug) Serial.println("SPEED 3");
          break;
        case IR_BUTTON_4:
          remoteSpeed1 = 255;
          remoteSpeed2 = -255;
          if (debug) Serial.println("SPEED 4");
          break;
      }
    }
    else {
      if (debug) Serial.println("NOTHING");
    }
  }
}
