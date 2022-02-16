#include <Arduino.h>
#include <JPEGDEC.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

#define FS_NO_GLOBALS
#include <FS.h>
#include "SPIFFS.h"

#include "Grayscale_IL0398.h"
#include "Web_Fetch.h"

#define WIFI_SSID "BELL502"
#define PASSWORD "2167ACD3E6AF"

#define SUN_RADIUS 200
#define IMG_SIZE 512
#define X_OFFSET -25
#define Y_OFFSET -35
#define DISPLAY_WIDTH 300
#define DISPLAY_HEIGHT 400

#define EPD_CS     5
#define EPD_DC     2 
#define SRAM_CS    -1
#define EPD_RESET   0 
#define EPD_BUSY    4 
#define EPD_DISPLAY_BUS &SPI1

JPEGDEC jpeg;
Grayscale_IL0398 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
    
int JPEGDraw(JPEGDRAW *pDraw)
{
  int x = pDraw->x;
  int y = pDraw->y;
  int w = pDraw->iWidth;
  int h = pDraw->iHeight;

  for(int i = 0; i < w * h; i++) {
    pDraw->pPixels[i] = (pDraw->pPixels[i] & 0x7e0) >> 5; // extract just the six green channel bits.
  }

  // dither
  for(int16_t j = 0; j < h; j++) {
    for(int16_t i = 0; i < w; i++) {
      int8_t oldPixel = constrain(pDraw->pPixels[i + j * w], 0, 0x3F);
      int8_t newPixel = oldPixel & 0x38; // or 0x30 to dither to 2-bit directly. much improved tonal range, but more horizontal banding between blocks.
      pDraw->pPixels[i + j * w] = newPixel;
      int quantError = oldPixel - newPixel;      
      if(i + 1 < w) pDraw->pPixels[i + 1 + j * w] += quantError * 7 / 16;
      if((i - 1 >= 0) && (j + 1 < h)) pDraw->pPixels[i - 1 + (j + 1) * w] += quantError * 3 / 16;
      if(j + 1 < h) pDraw->pPixels[i + (j + 1) * w] += quantError * 5 / 16;
      if((i + 1 < w) && (j + 1 < h)) pDraw->pPixels[i + 1 + (j + 1) * w] += quantError * 1 / 16;
    } 
  } 
  
  for(int16_t i = 0; i < w; i++) {
    for(int16_t j = 0; j < h; j++) {
      const int16_t xAbs = x+i, yAbs = y+j;
      switch (constrain(pDraw->pPixels[i + j * w] >> 4, 0, 3)) {
        case 0:
          if(inSpace(xAbs, yAbs))
            display.writePixel(xAbs, yAbs, EPD_WHITE);
          else
            display.writePixel(xAbs, yAbs, EPD_BLACK);
          break;
        case 1:
          display.writePixel(xAbs, yAbs, EPD_DARK);
          break;
        case 2:
          display.writePixel(xAbs, yAbs, EPD_LIGHT);
          break;
        case 3:
          display.writePixel(xAbs, yAbs, EPD_WHITE);
          break;
      } 
    } 
  } 
  
  return 1;
}

bool inSpace(int16_t x, int16_t y) {
  int center = IMG_SIZE / 2;
  return SUN_RADIUS < sqrt(pow(x-center-X_OFFSET, 2) + pow(y-center-Y_OFFSET, 2));
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  display.begin();

// Initialise SPIFFS
  if (!SPIFFS.begin(1)) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  WiFi.begin(WIFI_SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

if (SPIFFS.exists("/M81.jpg") == true) {
    Serial.println("For test only, removing file");
    SPIFFS.remove("/M81.jpg");
  }
} 

void loop() {
  uint32_t t = millis();

  // Fetch the jpg file from the specified URL, examples only, from imgur
  bool loaded_ok = getFile("https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0131.jpg", "/M81.jpg"); // Note name preceded with "/"
  //bool loaded_ok = getFile("https://i.imgur.com/OnW2qOO.jpg", "/F35.jpg");

  t = millis() - t;
  if (loaded_ok) { Serial.print(t); Serial.println(" ms to download"); }

  t = millis();
  int i;
  long lTime;
  File jpgFile = SPIFFS.open("/M81.jpg");
  if(!jpgFile){
    Serial.println("Failed to open file for reading");
    return;
  }
  uint32_t imageSize = jpgFile.size();
  char* pixels = (char *)malloc(imageSize);
  uint32_t nBytes = jpgFile.readBytes(pixels, imageSize);

  if (jpeg.openFLASH((uint8_t*)pixels, nBytes, JPEGDraw)) {
    lTime = micros();
    if (jpeg.decode(X_OFFSET, Y_OFFSET, 0)) {
    // if (jpeg.decode(0, 0, 0)) {
      lTime = micros() - lTime;
    }
    jpeg.close();
  }
  display.display();

  t = millis() - t;
  Serial.print(t); Serial.println(" ms to draw to TFT");

  while(1) yield();
} 