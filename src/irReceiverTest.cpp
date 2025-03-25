#include "MeMCore.h"
#include <SoftwareSerial.h>
#include <Wire.h>

MeIR ir;

#define IR 13
#define IRREMOTE 14
#define INFRARED 16
#define IRREMOTECODE 18

void setup()
{
  ir.begin();
  Serial.begin(9600);
}

void loop()
{
    // Serial.println("hello");
    // Serial.println(ir.decode());
    delay(500);
    static long time = millis();
    if (ir.decode()) {
      uint32_t value = ir.value;
      time = millis();

      switch (value >> 16 & 0xff)
      {
        case IR_BUTTON_A:
            Serial.println("A");
            break;
        case IR_BUTTON_B:
            Serial.println("B");
            break;
        case IR_BUTTON_C:
            Serial.println("C");
            break;
        case IR_BUTTON_PLUS:
            Serial.println("+");
            break;
        case IR_BUTTON_MINUS:
            Serial.println("-");
            break;
        case IR_BUTTON_NEXT:
            Serial.println("NEXT");
            break;
        case IR_BUTTON_PREVIOUS:
            Serial.println("PREV");
            break;
        case IR_BUTTON_9:
            Serial.println("9");
            break;
        case IR_BUTTON_8:
            Serial.println("8");
            break;
        case IR_BUTTON_7:
            Serial.println("7");
            break;
        case IR_BUTTON_6:
            Serial.println("6");
            break;
        case IR_BUTTON_5:
            Serial.println("5");
            break;
        case IR_BUTTON_4:
            Serial.println("4");
            break;
        case IR_BUTTON_3:
            Serial.println("3");
            break;
        case IR_BUTTON_2:
            Serial.println("2");
            break;
        case IR_BUTTON_1:
            Serial.println("1");
            break;
      }
    }
    else if (millis() - time > 120)
    {
        Serial.println("STOP");
        time = millis();
    }
}
