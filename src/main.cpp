#define LGFX_USE_V1

#include <LovyanGFX.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <WebServer.h>
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

//****** Variable settings *********
String connectedSSID = "";
long connectedRSSI = 0;
long intTemp = 0;
long binLevel = 5000;
bool dataExists = false;
boolean barcodeMatch = false;
boolean categoryMatch = false;
boolean nameContainsOzOrRelated = false;
String data;
String deviceType;
String deviceClient;
String deviceLocation;
String productName;
String productBrand;
String productCategory;

unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
const long interval = 900000; // 15 minutes in ms


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


// Supabase API information 
const char* supabaseURL = "https://msgnfrsepfjphmeosenp.supabase.co/rest/v1/hpu_scans"; //new beta_test_table - HPU
const char* supabaseURL2 = "https://msgnfrsepfjphmeosenp.supabase.co/rest/v1/DeviceStatus"; //device status table
const char* supabaseLKP = "https://msgnfrsepfjphmeosenp.supabase.co/rest/v1/lkp_master_barcodes_list?select=*";
const char* supabaseAPIKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im1zZ25mcnNlcGZqcGhtZW9zZW5wIiwicm9sZSI6ImFub24iLCJpYXQiOjE2OTI2NDAxNjksImV4cCI6MjAwODIxNjE2OX0.bhWa5q496hfO8Rq-D3mrDwsDqUiUMHElspQ1_BghUZw"; 
const char* filename = "/supabase_data.json"; // File path in SPIFFS

const long interval2 = 300000; // Interval set to 5 minutes (300000 milliseconds)

void getSupabaseData() {
    if (data.isEmpty()) {
        Serial.println("Error: Scanned data is empty.");
        return; // Early exit if data is empty.
    }

    if (data.startsWith("http://") || data.startsWith("https://")) {
        lcd.clearDisplay();
        //M5.Spk.PlaySound(dingdong, sizeof(dingdong)); 
        //displayRescanMessage();
        delay(3000);
        return; // Early exit for URL data.
    }

    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }

    File file = SPIFFS.open("/barcodes.txt", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    bool dataExists = false;
    data.trim(); // Ensure data is trimmed before use

    Serial.println("Starting to read the file:");
    Serial.print("Scanned data: '");
    Serial.print(data);
    Serial.println("'");

    while (file.available()) {
        String barcode = file.readStringUntil('\n');
        barcode.trim(); // Clean up the barcode string

        Serial.print("Read barcode: '");
        Serial.print(barcode);
        Serial.println("'");

        if (!barcode.isEmpty() && barcode.indexOf(data) != -1) { // Ensure non-empty barcode and substring match
            Serial.println("Match found: " + barcode);
            dataExists = true;
            break;
        }
    }

    file.close();

    if (!dataExists) {
        Serial.print("No match found for barcode: '");
        Serial.print(data);
        Serial.println("'");
    }
}

void fetchDataAndWriteToFile() {
    Serial.println("Fetching data from Supabase...");

    HTTPClient http;
    http.begin(supabaseLKP);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabaseAPIKey);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println("Data fetched successfully.");

        if (!SPIFFS.begin()) {
            Serial.println("An Error has occurred while mounting SPIFFS");
            return;
        }

        File file = SPIFFS.open("/barcodes.txt", FILE_WRITE);
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }

        DynamicJsonDocument doc(10240); // Adjust according to your data
        deserializeJson(doc, payload);
        JsonArray array = doc.as<JsonArray>();

        for (JsonVariant v : array) {
            file.println(v["barcode"].as<String>());
        }

        file.close();
        Serial.println("File written successfully");
    } else {
        Serial.print("Error during HTTP request: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void fetchDataTask(void *parameter) {
    for(;;) { // Infinite loop for the task
        unsigned long currentMillis = millis();
        static unsigned long previousMillis = 0;

        if (currentMillis - previousMillis >= interval2) {
            previousMillis = currentMillis;
            Serial.println("Fetching and writing data...");
            fetchDataAndWriteToFile(); // Fetch and store data every 5 minutes
            Serial.println("Data fetched successfully.");
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Delay a bit to avoid hogging the CPU
    }
}

void fetchProductDetails() {
    HTTPClient http;
    String apiKey = "a595b6f89e81c6ea6ed66c27bdf1b393b720a0aa948f90d780a295ebe4149b1f";
    String requestURL = "https://go-upc.com/api/v1/code/" + data + "?key=" + apiKey;

    http.begin(requestURL);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        http.end(); // Close the connection to free resources

        DynamicJsonDocument doc(2048); // Adjust based on your JSON response size
        deserializeJson(doc, response);
        
        // Extract barcode details from the API
        productName = doc["product"]["name"].as<String>();
        productBrand = doc["product"]["brand"].as<String>();
        productCategory = doc["product"]["category"].as<String>();
        productName.toLowerCase(); // Convert to lowercase for easier comparison
        productCategory.toLowerCase();

        // List of categories to check against, in lowercase
        String categories[] = {"coffee", "drink", "drinks", "soda", "juice", "water", "beverages", "milk", "tea", "beer", "wine", "liquor", "alcoholic"};
        


        // Check if productCategory is in the list of specified categories
        for (int i = 0; i < (sizeof(categories) / sizeof(categories[0])); i++) {
            if (productCategory.indexOf(categories[i]) != -1) {
                categoryMatch = true;
                break; // Exit the loop if a match is found
            }
        }

        // Check if productName contains "oz", "ounce", "ounces", etc.
        String units[] = {"oz", "ounce", "ounces", "fl oz", "ml", "liter"};
        for (int i = 0; i < (sizeof(units) / sizeof(units[0])); i++) {
            if (productName.indexOf(units[i]) != -1) {
                nameContainsOzOrRelated = true;
                break; // Exit the loop if a match is found
            }
        }

        // Output the result based on categoryMatch and nameContainsOzOrRelated
        if (categoryMatch && nameContainsOzOrRelated) {
            Serial.println("Match found: " + productName);
        } else {
            Serial.println("No match found.");
        }
    } else {
        Serial.print("HTTP GET request failed with error: ");
        Serial.println(httpResponseCode);
    }
}

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

  // Begin Wifi Configurations //
    WiFi.mode(WIFI_STA);
    WiFiManager wm;
            lcd.clearDisplay();
            lcd.fillScreen(TFT_YELLOW);
            lcd.setTextDatum(MC_DATUM);
            lcd.setTextColor(TFT_BLACK);
            lcd.setTextSize(3);
            lcd.print("Scan Wifi. . . .");
    //wm.resetSettings();  //Remove after testing          
    //******** Add WiFi Ready Screen Here M5.Lcd.drawBitmap(0, 0, 320, 240, (uint16_t *)wifi_ready);

    bool res;
    res = wm.autoConnect("TopperStopperAP","recycleit");

    String ssid = wm.getWiFiSSID();
      if(!res) 
        {
          lcd.print("Failed to connect and timedout.  Restarting. . .");
        }
      else
        {
            lcd.clearDisplay();
            lcd.fillScreen(TFT_GREEN);
            lcd.setTextDatum(MC_DATUM);
            lcd.setTextColor(TFT_BLACK);
            lcd.setTextSize(3);
            lcd.print("Wifi Connected!");
          Serial.println("Connected to the configured WiFi");
          connectedSSID = WiFi.SSID();
          connectedRSSI = WiFi.RSSI();
        }
    fetchDataAndWriteToFile();
    Serial.println("Inital barcode fetch to SPIFFS. . ."); // Initial fetch
  
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreate(imageTaskcode, "Image Task", 10000, NULL, 1, &imageTaskHandle);
   xTaskCreatePinnedToCore(
        fetchDataTask,   /* Task function */
        "FetchDataTask", /* Name of task */
        10000,           /* Stack size of task */
        NULL,            /* parameter of the task */
        1,               /* priority of the task */
        NULL,            /* Task handle to keep track of created task */
        0);              /* pin task to core 0 */                 
  xTaskCreate(scanTaskcode, "Scan Task", 10000, NULL, 2, &scanTaskHandle); // Higher priority
}

void loop(){}



