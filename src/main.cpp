#define LGFX_USE_V1

#include <LovyanGFX.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Arduino.h>
#include "terminal8048.h"

#include "tswift_ad.h"
#include "coke_ad.h"
#include "pepsi_ad.h"
#include "coke_ad2.h"


#define SCREEN_W 800
#define SCREEN_H 480

// microSD card
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12
#define SD_CS 10
 

#define SCANNER_TX 43
#define SCANNER_RX 44

const uint16_t* images[] = {coke_ad, tswift_ad, pepsi_ad};
int imageCount = sizeof(images) / sizeof(images[0]);

LGFX lcd;

void setup(){
  lcd.init();
  lcd.setBrightness(150);
  lcd.setSwapBytes(true); // Adjust depending on your LCD driver
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
}


void loop(){
lcd.pushImageDMA(0, 0, 800, 480, tswift_ad);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, pepsi_ad);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, coke_ad2);
delay(5000);
}