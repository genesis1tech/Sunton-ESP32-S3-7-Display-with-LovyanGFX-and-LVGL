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
 

#define SCANNER_TX 20
#define SCANNER_RX 19


const uint16_t* images[] = {coke_recycle_gen, coke_ad, tswift_ad, pepsi_ad, coke_ad2, coke_recycle};
int imageCount = sizeof(images) / sizeof(images[0]);

LGFX lcd;
LGFX_Sprite sprite(&lcd);
LGFX_Sprite txtLayer(&lcd);
LGFX_Sprite background(&lcd);

//Tasks
TaskHandle_t imageTaskHandle = NULL;
TaskHandle_t scanTaskHandle = NULL;
SemaphoreHandle_t semaphore;

void scanTaskcode(void * pvParameters) {
    for (;;) {
        if (Serial2.available()) {
            // Notify the image task to stop current operation
            xTaskNotify(imageTaskHandle, 0x01, eSetBits);

            lcd.clearDisplay();
            lcd.fillScreen(TFT_GREEN);
            lcd.setTextDatum(MC_DATUM);
            lcd.setTextColor(TFT_BLACK);
            lcd.setTextSize(3);
            String line = Serial2.readStringUntil('\n');
            Serial.println(line); 
            lcd.println(line);
            vTaskDelay(pdMS_TO_TICKS(2700)); // Simulate processing time

            // Optional: Notify again if you want the image task to continue
            xTaskNotify(imageTaskHandle, 0x00, eSetBits);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Check for new data every 10ms
    }
}


void imageTaskcode(void * pvParameters) {
    uint32_t ulNotificationValue;
    int currentIndex = 0;  // Track the current image index

    for (;;) {
        // Display the image at the current index
        lcd.pushImageDMA(0, 0, 800, 480, images[currentIndex]);
        //txtLayer.pushImageDMA(0, 0, 480, 50, scantext);
        //txtLayer.pushSprite(0, 272);
        // Delay to show image, delay is interruptible by notifications
        for (int delayCount = 0; delayCount < 500; delayCount++) {
            if (xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, pdMS_TO_TICKS(10))) {
                // Check if there is a notification to stop
                if (ulNotificationValue == 1) {
                    // Clear notification and wait for the go-ahead to resume
                    ulNotificationValue = 0;
                    xTaskNotifyWait(0x00, ULONG_MAX, NULL, portMAX_DELAY);
                }
                break;
            }
        }
        // Move to next image
        currentIndex = (currentIndex + 1) % imageCount;
    }
}


void setup(){
  lcd.init();
  Serial2.begin(9600, SERIAL_8N1, SCANNER_RX, SCANNER_TX);
  lcd.setBrightness(200);
  lcd.setSwapBytes(true); // Adjust depending on your LCD driver
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  sprite.createSprite(800, 480);
  txtLayer.createSprite(480,100);
  
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreate(imageTaskcode, "Image Task", 10000, NULL, 1, &imageTaskHandle);                 
  xTaskCreate(scanTaskcode, "Scan Task", 10000, NULL, 2, &scanTaskHandle); // Higher priority
}

void loop(){}

