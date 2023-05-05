#ifndef LOADING_H
#define LOADING_H

#include "loading.h"


#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels



#if (SSD1306_LCDHEIGHT != 64)


#endif

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//struct loadbar;


//Bytt loadbar med stor L Loadbar
struct loadbar {
  int x, y, width, height;
  loadbar(int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height) {}
  void draw(float progress, Adafruit_SSD1306& display) {

    progress = progress >  100 ?  100 : progress;

    progress = progress <  0 ?  0 : progress;

    float bar = ((float)(width - 4) / 100) * progress;

    display.drawRect(x, y, width, height, WHITE);



    display.fillRect(x + 2, y + 2, bar, height - 4, WHITE);
    // Display progress text

    if (height >= 15) {

      display.setCursor((width / 2) - 3, y + 5);

      display.setTextSize(1);

      display.setTextColor(WHITE);

      if (progress >= 80) {
        display.setTextColor(BLACK, WHITE);  // 'inverted' text
      } else if (progress < 80) {
        display.setTextColor(WHITE, BLACK);  // 'inverted' text
      }

      display.print(progress);

      display.print("%");
    }
  }
};




#endif