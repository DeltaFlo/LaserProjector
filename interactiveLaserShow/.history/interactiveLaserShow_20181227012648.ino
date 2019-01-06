#include "Laser.h"

#define START 0xff
#define END 0xfe

Laser laser(5);
unsigned short pointsBuffer[300] = {
    0x17c,
    0x3e8,
    0x8000,
    0x0,
    0x17c,
    0x3e8,
    0x82f9,
    0x0,
    0x8e,
    0x14d,
    0x826b,
    0x14d,
};
boolean onRoutine;
int idx;
int idxLimit;
char buf[4];

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);
    laser.init();
    onRoutine = true;
    idx = 0;
    idxLimit = 12;
}

void loop()
{
    if (onRoutine)
    {
        digitalWrite(LED_BUILTIN, LOW);
        if (Serial.available() > 0)
        {
            if (Serial.read() == START)
                switchToNoRoutine();
        }
        else
        {
            unsigned short posX = pointsBuffer[idx];
            idx++;
            unsigned short posY = pointsBuffer[idx];
            idx++;

            laser.sendto(posX & 0x7fff, posY & 0xfff);

            if (idx == idxLimit)
                idx = 0;
        }
    }

    else
    {
        digitalWrite(LED_BUILTIN, HIGH);
        if (idx >= 300)
            switchToRoutine();

        char initialInput;
        if (Serial.available() > 0)
        {
            initialInput = Serial.read();
            if (initialInput == END)
                switchToRoutine();
        }
        if (Serial.available() >= 3)
        {
            unsigned short tmpInput;
            tmpInput = initialInput << 8;
            tmpInput += Serial.read();
            pointsBuffer[idx] = tmpInput;
            idx++;
            tmpInput = Serial.read() << 8;
            tmpInput += Serial.read();
            pointsBuffer[idx] = tmpInput;
            idx++;

            unsigned short posX = pointsBuffer[idx - 2];
            unsigned short posY = pointsBuffer[idx - 1];
            laser.sendto(posX & 0x7fff, posY & 0xfff);
            // Serial.write("U / X: ");
            // Serial.write(posX & 0xfff);
            // Serial.write("Y: ");
            // Serial.write(posY & 0xfff);
            // Serial.write("\n");
        }
    }

    delay(500);
}

void switchToNoRoutine()
{
    onRoutine = false;
    idx = 0;
}

void switchToRoutine()
{
    onRoutine = true;
    idxLimit = idx;
    idx = 0;
}