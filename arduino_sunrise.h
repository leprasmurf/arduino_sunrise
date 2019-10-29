
/* Reference links
    Arduino pro-mini 5v 16mhz details:  https://learn.sparkfun.com/tutorials/using-the-arduino-pro-mini-33v/what-it-is-and-isnt
    Arduino pro-mini 5v 16mhz schematic:  https://cdn.sparkfun.com/datasheets/Dev/Arduino/Boards/Arduino-Pro-Mini-v14.pdf
    TFT screen wiring:  https://learn.adafruit.com/1-8-tft-display/breakout-wiring-and-test
    RTC wiring:  https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/wiring-it-up
    RTC library:  https://github.com/adafruit/RTClib
    LED Basic Usage:  https://github.com/FastLED/FastLED/wiki/Basic-usage
*/

/* Hardware Libraries
 */
//#include <Adafruit_GFX.h>    // LCD - Core graphics library
//#include <Adafruit_ST7735.h> // LCD - Hardware-specific library
#include <EEPROM.h>          // Arduino's harddrive
#include "RTClib.h"          // RTC
#include <SPI.h>             // LCD
#include <TFT.h>             // LCD
#include <Wire.h>            // RTC & LEDs

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>         // LEDs

/* RTC
    DS1307 Real Time Clock Breakout Board - connected via I2C and Wire.h
    https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/
*/
RTC_DS1307 rtc;

/* LED
    ALITOVE 16.4ft WS2812B Individually Addressable LED Strip Light 5050 RGB SMD 150 Pixels Dream Color Waterproof IP66 Black PCB 5V DC
    https://www.amazon.com/gp/product/B00ZHB9M6A/ref=oh_aui_search_detailpage?ie=UTF8&psc=1
*/
#define NUM_LEDS     150
#define DATA_PIN     6 // Digital (LED data pin)
#define TARGET_MIN   60 // Time (in minutes) prior to sunrise/sunset to start LED cycle

CRGB leds[NUM_LEDS];

/* Buttons
*/
#define BTN_OK       2 // Digital (button_ok)
#define BTN_LEFT     7 // Digital (button_left)
#define BTN_RIGHT    3 // Digital (button_right)
#define LONG_BTN_PRS 2 // Long press (in seconds)

/* TFT Screen
    1.8" Color TFT LCD display with MicroSD Card Breakout - ST7735R
    http://www.adafruit.com/products/358
*/
#define BACKGROUND   CRGB(0, 0, 0)
#define TFT_DC       8 // Digital
#define TFT_RST      9 // Digital
#define TFT_CS       10 // Digital
#define SD_CS        4 // Digital
// #define TFT_MOSI    11
/* TFT_SCLK is specified for faster LCD functionality
     https://learn.adafruit.com/1-8-tft-display/breakout-wiring-and-test
*/
// #define TFT_SCLK    13
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
TFT tft = TFT(TFT_CS, TFT_DC, TFT_RST);

struct RGB {
  byte r;
  byte g;
  byte b;
};

struct RGB bg_color = { 0, 0, 0 };
struct RGB clock_text_color = { 225, 0, 255 };
struct RGB punctuation_color = { 255, 255, 255 };

/* General
*/
bool setup_mode = false;
bool sunrise_mode = false;
bool sunset_mode = false;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
DateTime now_last;
DateTime now_now;

byte tmp_byte;

/*
   Notes:
    following DATE(1) format to 'd'
    Sunrise = DATE+1
    Sunset = DATE+2
*/
#define SETTINGS 14
char setting_entries_dict[SETTINGS] = {
  'H', 'M', 'S', 'p', 'Y', 'm', 'd', 'u', // Current DateTime
  'I', 'N', 'q', // Sunrise
  'J', 'O', 'r' // Sunset
};
byte setting_entry = 0;

