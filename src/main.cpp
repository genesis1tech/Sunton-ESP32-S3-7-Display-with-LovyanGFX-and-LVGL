#define LGFX_USE_V1

#include <LovyanGFX.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Arduino.h>
#include "terminal8048.h"

#include "swift_8bit.h"
#include "recycle1_8bit.h"
#include "pepsi_8bit.h"
#include "lower-8bit.h"



#define SCREEN_W 800
#define SCREEN_H 480

// microSD card
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12
#define SD_CS 10
 

#define SCANNER_TX 43
#define SCANNER_RX 44

const uint8_t* images[] = {swift_8bit, recycle1_8bit, pepsi_8bit, lower_8bit};
int imageCount = sizeof(images) / sizeof(images[0]);

LGFX lcd;
LGFX_Sprite sprite(&lcd);

//Tasks
TaskHandle_t imageTask;
TaskHandle_t scanTask;
//SemaphoreHandle_t semaphore;

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

/*
void loop(){
lcd.pushImageDMA(0, 0, 800, 480, coke_recycle_gen);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, coke_ad);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, tswift_ad);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, pepsi_ad);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, coke_ad2);
delay(5000);
lcd.pushImageDMA(0, 0, 800, 480, coke_recycle);
delay(5000);
}
*/
