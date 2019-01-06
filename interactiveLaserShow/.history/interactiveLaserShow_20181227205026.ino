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
    Serial.begin(115200);
    laser.init();
    laser.setScale(1);
    laser.setOffset(0, 0);
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

            laser.sendto(posX & 0x8fff, posY & 0xfff);
            // Serial.print(posX & 0xfff);
            // Serial.write(',');
            // Serial.print(posY);
            // Serial.write('\n');

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
                tmpInput = (static_cast<unsigned short>(popFromSerial())) << 8;
                tmpInput |= static_cast<unsigned short>(popFromSerial()) & 0x00ff;
                // unsigned short posX = tmpInput;
                unsigned short posY = tmpInput;
                tmpInput = (static_cast<unsigned short>(popFromSerial())) << 8;
                tmpInput |= static_cast<unsigned short>(popFromSerial()) & 0x00ff;
                // unsigned short posY = tmpInput;
                unsigned short posX = tmpInput;

                bool isValidated = validateXY(posX, posY);
                if (isValidated) {
                    if(validateXY(posY, posX)) {
                        isValidated = true;
                        tmpInput = posX;
                        posX = posY;
                        posY = tmpInput;
                    }
                }

                Serial.print(posX);
                Serial.write(',');
                Serial.print(posY);
                Serial.write('\n');

                if (isValidated)
                {
                    pointsBuffer[idx] = posX;
                    pointsBuffer[idx+1] = posY;
                    idx += 2;

                    laser.sendto(posX & 0x8fff, posY & 0xfff);
                }
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
    // return true;
    return ((x & (0b0111 << 12)) == 0) && (y & (0b1111 << 12) == 0);
}