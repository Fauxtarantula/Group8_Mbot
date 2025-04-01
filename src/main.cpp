#include "MeMCore.h"
#include "SoftwareSerial.h"

/*
REMOTE CONTROL BUTTONS:

SWITCHING MODES:
B: manual mode
C: auto mode

IN AUTO:
A: restarts the program when robot is stopped (stop by IR, return or hunt for IR)
D: hunt for IR
E: return to base

IN MANUAL:
UP/DOWN: move forward/back
LEFT/RIGHT: turn left/right
SETTINGS: stop
1/2/3/4: change speed (must change directions to see the effect)
*/

// pins
const int loadSensor = A2;
const int IRSensor = A3;

// constants
const int speed1 = 150;
const int speed2 = -150;
const int loadThresh = 650;
const int IRThresh = 300;
const int ultraThresh = 10;

// sensors
int load, line, IR, ultra = 0;

// extras
// bools
int manual = 0; // mode
int stop = 0; // stop moving when see IR array
int ret = 0; // return to original
int hunt = 0; // hunt for IR
int remoteStop = 0; // stop by remote
int decode = 0; // checks if IR remote is sending anything
int firstManual, firstAuto = 1; // checks if it is the first loop after switching modes
//others
int turnOffset = 0; // measures the number of turns done (+1 for right, -1 for left)
int returnSpeed1 = 100; // speed when returning to original position
int returnSpeed2 = -100;
int remoteSpeed1 = 100; // speed in manual mode
int remoteSpeed2 = -100;
int delayDur = 1000; // delays in between resetting movements

int debug  = 1; // 1 to print, 0 to not

MeIR ir;
MeBuzzer buzzer;
MeDCMotor motor1(M1);
MeDCMotor motor2(M2);
MeLineFollower lineFinder(PORT_1);
MeUltrasonicSensor ultraSensor(PORT_4);

void turn(bool dir) { // 90 degree turn left == 1, right == 0
  int turnDur = 347;
  int turnSpeed = 230;

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

void checkMode(int decode) { // updates mode
  if (decode) {
    uint32_t value = ir.value;

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

void checkModeLoop() { // updates mode
  if (ir.decode()) {
    uint32_t value = ir.value;

    switch (value >> 16 & 0xff) {
      case IR_BUTTON_B: // switch to manual
        if (debug) Serial.println("SWITCH TO MANUAL");
        manual = 1;
        stop = 0; // combined for the stop, return and hunt
        remoteStop = 0;
        ret = 0;
        hunt = 0;
        // return 1;
        break;
    }
  }
}

void checkStop(int decode) { // updates stop if IR array is detected
  if (decode) {
    uint32_t value = ir.value;

    switch (value >> 16 & 0xff) {
      case IR_BUTTON_SETTING: // stop moving
        if (debug) Serial.println("REMOTE STOP");
        remoteStop = 1;
        break;
    }
  }
}

void checkReturn(int decode) { // updates return to original position
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

void checkHunt(int decode) { // updates hunt for IR array
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

void checkRestart() { // updates restart the cleaning program after either stopped by IR, returned to original pos or hunted for IR
  if (ir.decode()) {
    uint32_t value = ir.value;

    switch (value >> 16 & 0xff) {
      case IR_BUTTON_A: // restart program
        if (debug) Serial.println("RESTART");
        stop = 0; // combined for the stop, return and hunt
        remoteStop = 0;
        ret = 0;
        hunt = 0;
        break;
    }
  }
}

void turnReset() { // makes the bot turn to face the original angle
  // we assume the robot starts in the bottom right corner
  turnOffset -= 2; // make it turn to face the back
  turnOffset %= 4; // wraparound to avoid unnecessary turns
  if (turnOffset == 3) turnOffset = -1; // take the path with fewer number of turns
  else if (turnOffset == -3) turnOffset = 1;

  while (turnOffset != 0) {
    if (turnOffset > 0) turn(1); // if facing right turn left
    else turn(0); // if facing left turn right
  }
  motor1.stop();
  motor2.stop();
  delay(500);
}

void moveReset() { // resets the robot translationally in forward axis using the black line
  line = lineFinder.readSensors();
  while (line == 3) { // move forward until see black line
    line = lineFinder.readSensors();
    motor1.run(returnSpeed1);
    motor2.run(returnSpeed2);
  }
  motor1.stop();
  motor2.stop();
  delay(delayDur);

  while (line != 0) {  // turn to be straight, most of the time this does nothing because the sensors are too close
    line = lineFinder.readSensors();
    motor1.run(-returnSpeed1);
    motor2.run(returnSpeed2);
  }
  motor1.stop();
  motor2.stop();
  delay(delayDur);
}

void setup() {
  Serial.begin(9600);
  ir.begin();
  pinMode(loadSensor, INPUT);
  pinMode(IRSensor, INPUT);
}

void loop() {
  load = analogRead(loadSensor); // load sensor, higher means more weight
  line = lineFinder.readSensors(); // 3 is both white, 2 is right black, 1 is left black, 0 is both black
  IR = analogRead(IRSensor); // IR sensor, lower means IR detected
  ultra = ultraSensor.distanceCm(); // distance in cm

  decode = ir.decode(); // check if anything is received by the built-in IR receiver
  checkMode(decode);

  if (!manual) { // automatic mode
    firstManual = 1;
    if (firstAuto) { // after switching modes beep
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
      motor1.stop();
      motor2.stop();
      delay(50);
      motor1.run(-speed1);
      motor2.run(-speed2);
      delay(200);
      motor1.stop();
      motor2.stop();
      delay(50);
      if (line == 2) turn(1); // turn left if right sensor sees black
      else turn(0); // turn right if left or both sensors see black
      // delay(50);
      // motor1.run(-speed1);
      // motor2.run(-speed2);
      // delay(200);
    }

    checkStop(decode);
    if ((IR < IRThresh) | remoteStop) { // if IR led sees IR array stop 
      stop = 1;
      while (stop == 1) { // wait for restart
        motor1.stop();
        motor2.stop();
        checkRestart();
        checkModeLoop();
        if (manual) break;
      }
    }

    if (ultra < ultraThresh) {
      motor1.stop();
      motor2.stop();
      delay(50);
      turn(0); // turn if see object
    }

    motor1.run(speed1);
    motor2.run(speed2);

    checkReturn(decode);
    if (ret) { // return to original position
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      turnReset(); // turn to face original angle
      moveReset(); // reset in the y-axis
      turn(1); // turn to x-axis
      delay(delayDur); 
      moveReset(); // reset in the x-axis
      turn(1); // turn to face the front
      delay(delayDur);

      /*// y-axis alignment
      while (line == 3) { // move forward until see black line
        line = lineFinder.readSensors();
        motor1.run(returnSpeed1);
        motor2.run(returnSpeed2);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);
      while (line != 0) {  // turn to be straight
        line = lineFinder.readSensors();
        motor1.run(-returnSpeed1);
        motor2.run(returnSpeed2);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      // x-axis alignment
      turn(1); // turn right to align in the other axis
      delay(delayDur);
      line = lineFinder.readSensors();
      while (line == 3) { // move forward until see black line
        line = lineFinder.readSensors();
        motor1.run(returnSpeed1);
        motor2.run(returnSpeed2);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);
      while (line != 0) {  // turn to be straight
        line = lineFinder.readSensors();
        motor1.run(-returnSpeed1);
        motor2.run(returnSpeed2);
      }
      motor1.stop();
      motor2.stop();
      delay(delayDur);

      turn(1); // turn to face the front
      motor1.stop();
      motor2.stop();
      delay(delayDur);*/

      while (ret) {
        checkRestart();
        checkModeLoop();
        if (manual) break;
      }
    }

    checkHunt(decode);
    if (hunt) { // hunt for IR array
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

      while (hunt) {
        checkRestart();
        checkModeLoop();
        if (manual) break;
      }
    }
  }
  else {
    if (debug) Serial.print("MANUAL ");
    firstAuto = 1;
    if (firstManual) { // after switching modes beep
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
        case IR_BUTTON_F:
          motor1.run(255);
          motor2.run(-255);
          delay(1000);
          motor1.run(-255);
          motor2.run(255);
          delay(1000);
          motor1.run(255);
          motor2.run(-255);
          delay(1000);
          motor1.run(255);
          motor2.run(255);
          delay(1000);
          motor1.run(-255);
          motor2.run(-255);
          delay(1000);
          motor1.stop();
          motor2.stop();
          if (debug) Serial.println("FUN");
          break;
      }
    }
    else {
      if (debug) Serial.println("NOTHING");
    }
  }
}
