
#include <SPI.h>

#include "fonts.h"

#define DELAY_INTERVAL 160
#define FLASH_INTERVAL 80
#define FONT_SPACE 2
#define SS_SIZE 3
char *DisplayWords[] = {"\033\036\037 \033\034\035 ", "\031\032!"};
const int INSTANCE_CNT = 2;
const byte SS_SET[] = {10, 9, 8, 7, 6, 5, 4, 3};

#define MIN_ASCII 25
#define BIT_CNT (SS_SIZE << 3)
const byte NOOP = 0x0;
const byte DECODEMODE = 0x9;
const byte INTENSITY = 0xA;
const byte SCANLIMIT = 0xB;
const byte SHUTDOWN = 0xC;
const byte DISPLAYTEST = 0xF;

byte buffer[SS_SIZE << 3] = {0};
int switchFlag = 1;
int instanceIdx = 0;
byte index = 0;
byte TOTAL_LEN;
char *DisplayWord;
unsigned long prevTime = 0;
byte addBlank = 0;

void switchText(int idx, boolean needClear) {
  byte k, j, i, b, mask = 1;
  if (needClear) {
    for (i = 0; i < 8; i++) {
      for (k = 0; k < SS_SIZE; k++) {
        for (j = 0; j < 8; j++) {
          buffer[(k << 3) + j] |= mask;
          max7219(SS_SET[k], j + 1, buffer[(k << 3) + j]);
        }
      }
      mask <<= 1;
      delay(FLASH_INTERVAL);
    }
  
    for (k = 0; k < BIT_CNT; k++) {
      buffer[k] = 0xFF;
    }

    mask = 0xFF;
    for (i = 0; i < 8; i++) {
      mask >>=1;
      for (k = 0; k < SS_SIZE; k++) {
        for (j = 0; j < 8; j++) {
          buffer[(k << 3) + j] &= mask;
          max7219(SS_SET[k], j + 1, buffer[(k << 3) + j]);
        }
      }
      delay(FLASH_INTERVAL);
    }
  }
  DisplayWord = DisplayWords[idx];
  char *str = DisplayWord;
  while(*str) {
    str++;
  }
  TOTAL_LEN = ((int)(str - DisplayWord)) << 3;
  index = 0;
  prevTime = millis();
}

void max7219(byte pin, byte reg, byte data) {
  digitalWrite(pin, LOW);
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(pin, HIGH);
}

void setup() {
  byte k, i;

  pinMode(2, INPUT);
  for (k = 0; k < SS_SIZE; k++) {
    pinMode(SS_SET[k], OUTPUT);
    digitalWrite(SS_SET[k], HIGH);
  }

  SPI.begin();
  for (k = 0; k < SS_SIZE; k++) {
    max7219(SS_SET[k], SCANLIMIT, 7);
    max7219(SS_SET[k], DECODEMODE, 0);
    max7219(SS_SET[k], INTENSITY, 4);
    max7219(SS_SET[k], DISPLAYTEST, 0);
    max7219(SS_SET[k], SHUTDOWN, 1);
      
    for (i = 0; i < 8; i++) {
      max7219(SS_SET[k], i + 1, 0);
    }
  }
  
  switchText(0, false);
}

void loop() {
  byte j, k, chr;
  int switchInput = digitalRead(2);
  unsigned long currTime = 0;
  if (switchInput == 1 && switchInput != switchFlag) {
    switchFlag = switchInput;
    instanceIdx = (instanceIdx + 1) % INSTANCE_CNT;
    switchText(instanceIdx, true);
  } else if (switchInput != switchFlag) {
    switchFlag = switchInput;
  }

  for (k = 0; k < SS_SIZE; k++) {
    for (j = 0; j < 8; j++) {
      max7219(SS_SET[k], j + 1, buffer[k * 8 + j]);
    }
  }

  currTime = millis();
  if (currTime - prevTime >= DELAY_INTERVAL) {
    prevTime = currTime;

    for (k = 0; k < BIT_CNT - 1; k++) {
      buffer[k] = buffer[k + 1];
    }

    if (addBlank < FONT_SPACE) {
      buffer[BIT_CNT - 1] = 0;
      addBlank++;
    } else {
      chr = DisplayWord[index >> 3];
      if (chr < MIN_ASCII || chr > 127) {
        chr = ' ';
      }

      buffer[BIT_CNT - 1] = fonts[chr - MIN_ASCII][index & 7];
      index = (index + 1) % TOTAL_LEN;
      if ((index & 7) == 0) {
        addBlank = 0;
      }
    }
  }
}

