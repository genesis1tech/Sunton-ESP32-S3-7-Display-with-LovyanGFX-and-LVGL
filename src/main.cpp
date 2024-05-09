#define LGFX_USE_V1

#include <LovyanGFX.h>
#include <SPIFFS.h>
//#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Arduino.h>
//#include "M5_UNIT_8SERVO.h"
#include "terminal8048.h"



#include "tswift_ad.h"
#include "coke_ad.h"
#include "pepsi_ad.h"

#define SCREEN_W 800
#define SCREEN_H 480

// microSD card
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12
#define SD_CS 10

//Servo
//M5_UNIT_8SERVO unit_8servo;
//#define SDA_PIN 19
//#define SCL_PIN 20  

#define SCANNER_TX 43
#define SCANNER_RX 44
//SoftwareSerial scanner(SCANNER_RX, SCANNER_TX);


const uint16_t* images[] = {coke_ad, tswift_ad, pepsi_ad};
int imageCount = sizeof(images) / sizeof(images[0]);

LGFX lcd;
LGFX_Sprite sprite(&lcd);

//Tasks
TaskHandle_t imageTaskHandle = NULL;
TaskHandle_t scanTaskHandle = NULL;
SemaphoreHandle_t semaphore;

void listDir(fs::FS & fs, const char *dirname, uint8_t levels)
{
  //  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    //Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  //  i = 0;
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("FILE: ");
      Serial.print(file.name());
      //      lcd.setCursor(0, 2 * i);
      //      lcd.printf("FILE:%s", file.name());
      Serial.print("SIZE: ");
      Serial.println(file.size());
      //      lcd.setCursor(180, 2 * i);
      //      lcd.printf("SIZE:%d", file.size());
      //      i += 16;
    }

    file = root.openNextFile();
  }
}

int SD_init()
{

  if (!SD.begin(SD_CS))
  {
    Serial.println("Card Mount Failed");
    return 1;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No TF card attached");
    return 1;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("TF Card Size: %lluMB\n", cardSize);
  listDir(SD, "/", 2);
  return 0;
}

void imageTaskcode(void * pvParameters) {
    uint32_t ulNotificationValue;
    int currentIndex = 0;  // Track the current image index

    for (;;) {
        // Display the image at the current index
        lcd.pushImageDMA(0, 0, 800, 480, images[currentIndex]);
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

/*
void imageTaskcode(void * pvParameters) {
    uint32_t ulNotificationValue;
    for (;;) {
        ulNotificationValue = 0; // Reset notification value at start of the loop
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, portMAX_DELAY);

        for (int i = 0; i < imageCount; i++) {
            if (ulNotificationValue != 0) {
                // If a notification is received, process it and break if needed
                ulNotificationValue = 0;  // Clear the notification flag
                break;  // Break the inner loop to check the outer loop condition or wait for another notification
            }
            lcd.pushImageDMA(0, 0, 800, 480, images[i]);
            vTaskDelay(pdMS_TO_TICKS(5000)); // Display each image for 5 seconds
        }
    }
}
*/


/*
void imageTaskcode(void * pvParameters) {
    uint32_t ulNotificationValue;
    for (;;) {
        // Wait for a notification, clear on entry and exit, wait indefinitely
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, portMAX_DELAY);
        
        for (int i = 0; i < imageCount; i++) {
            if (ulNotificationValue != 0) {
                // Notification received, clear it and break the loop
                ulNotificationValue = 0;
                break;
            }
            lcd.pushImageDMA(0, 0, 800, 480, images[i]);
            vTaskDelay(pdMS_TO_TICKS(5000)); // Display each image for 5 seconds
        }
    }
}
*/


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
            lcd.println(line);
            vTaskDelay(pdMS_TO_TICKS(2700)); // Simulate processing time

            // Optional: Notify again if you want the image task to continue
            xTaskNotify(imageTaskHandle, 0x00, eSetBits);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Check for new data every 10ms
    }
}

void setup() {
    Serial.begin(115200);
    lcd.init();
    Serial2.begin(9600, SERIAL_8N1, SCANNER_RX, SCANNER_TX);
    lcd.setBrightness(255);
    lcd.setSwapBytes(true);

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    if (SD_init() == 1) {
        Serial.println("Card Mount Failed");
    } else {
        Serial.println("SD Card initialized successfully");
    }
    sprite.createSprite(800, 480);

    // Create tasks
    xTaskCreate(imageTaskcode, "Image Task", 10000, NULL, 1, &imageTaskHandle);
    xTaskCreate(scanTaskcode, "Scan Task", 10000, NULL, 2, &scanTaskHandle); // Higher priority
}

void loop(){

}

