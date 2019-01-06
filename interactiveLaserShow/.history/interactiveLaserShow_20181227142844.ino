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
int waitCnt;
char buf[100];
int bufLength;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);
    laser.init();
    onRoutine = true;
    idx = 0;
    waitCnt = 0;
    idxLimit = 12;
    bufLength = 0;
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
            else
            {
                buf[bufLength] = initialInput;
                bufLength++;
            }

            if ((Serial.available() + bufLength) >= 4)
            {
                unsigned short tmpInput;
                tmpInput = ((unsigned short)popFromSerial()) << 8;
                tmpInput += popFromSerial() & 0x00ff;
                pointsBuffer[idx] = tmpInput;
                idx++;
                tmpInput = ((unsigned short)popFromSerial()) << 8;
                tmpInput += popFromSerial() & 0x00ff;
                pointsBuffer[idx] = tmpInput;
                idx++;

                unsigned short posX = pointsBuffer[idx - 2];
                unsigned short posY = pointsBuffer[idx - 1];
                laser.sendto(posX & 0x7fff, posY & 0xfff);
                Serial.print(posX);
                Serial.write(',');
                Serial.print(posY);
                Serial.write('\n');
            }
            waitCnt = 0;
        }
        else
        {
            waitCnt++;
            if (waitCnt >= 100)
                switchToRoutine;
        }
    }

    // delay(100);
}

void switchToNoRoutine()
{
    onRoutine = false;
    idx = 0;
    waitCnt = 0;
    bufLength = 0;
}

void switchToRoutine()
{
    onRoutine = true;
    idxLimit = idx;
    idx = 0;
    waitCnt = 0;
    bufLength = 0;
}

char popFromSerial()
{
    if (bufLength > 0)
    {
        bufLength--;
        return buf[bufLength];
    }
    else
    {
        return Serial.read();
    }
}

boolean validateXY(unsigned short x, unsigned short y)
{
    return (x & 0x8000 == 0) && (y & 0xf000 == 0);
}