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
#include "coke_recycle.h"
#include "coke_recycle_gen.h"


#define SCREEN_W 800
#define SCREEN_H 480

// microSD card
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12
#define SD_CS 10
 

#define SCANNER_TX 43
#define SCANNER_RX 44

const uint16_t* images[] = {coke_recycle_gen, coke_ad, tswift_ad, pepsi_ad, coke_ad2, coke_recycle};
int imageCount = sizeof(images) / sizeof(images[0]);

LGFX lcd;
LGFX_Sprite sprite(&lcd);

//Tasks
TaskHandle_t imageTaskHandle = NULL;
TaskHandle_t scanTaskHandle = NULL;
SemaphoreHandle_t semaphore;

void imageTaskcode( void * pvParameters ){
 for (;;) {     
        for (int i = 0; i < imageCount; i++) {
            //xSemaphoreTake(semaphore, portMAX_DELAY);
            lcd.pushImageDMA(0, 0, 800, 480, images[i]);
            delay(8000); // Display each image for 8 seconds
            //xSemaphoreGive(semaphore);
            }
          }
  }

void setup(){
  lcd.init();
  lcd.setBrightness(200);
  lcd.setSwapBytes(true); // Adjust depending on your LCD driver
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  sprite.createSprite(800, 480);
  
  //semaphore = xSemaphoreCreateMutex();

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(imageTaskcode, "imageTask", 10000, NULL, 1, &imageTask, 0);                  
  delay(500); 
}

void loop(){}

