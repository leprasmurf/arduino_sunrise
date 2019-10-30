
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
#include <EEPROM.h> // Arduino's harddrive
#include "RTClib.h" // RTC
#include <SPI.h>    // LCD
#include <TFT.h>    // LCD
#include <Wire.h>   // RTC & LEDs

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h> // LEDs

/* RTC
    DS1307 Real Time Clock Breakout Board - connected via I2C and Wire.h
    https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/
*/
RTC_DS1307 rtc;

/* LED
    ALITOVE 16.4ft WS2812B Individually Addressable LED Strip Light 5050 RGB SMD 150 Pixels Dream Color Waterproof IP66 Black PCB 5V DC
    https://www.amazon.com/gp/product/B00ZHB9M6A/ref=oh_aui_search_detailpage?ie=UTF8&psc=1
*/
#define NUM_LEDS 150
#define DATA_PIN 6 // Digital (LED data pin)

CRGB leds[NUM_LEDS];

/* Buttons
*/
#define BTN_OK 2         // Digital (button_ok)
#define BTN_LEFT 7       // Digital (button_left)
#define BTN_RIGHT 3      // Digital (button_right)
#define LONG_BTN_PRESS 2 // Long press (in seconds)

/* TFT Screen
    1.8" Color TFT LCD display with MicroSD Card Breakout - ST7735R
    http://www.adafruit.com/products/358
*/
#define BACKGROUND CRGB(0, 0, 0)
#define TFT_DC 8  // Digital
#define TFT_RST 9 // Digital
#define TFT_CS 10 // Digital
#define SD_CS 4   // Digital
// #define TFT_MOSI    11
/* TFT_SCLK is specified for faster LCD functionality
     https://learn.adafruit.com/1-8-tft-display/breakout-wiring-and-test
*/
// #define TFT_SCLK    13
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
TFT tft = TFT(TFT_CS, TFT_DC, TFT_RST);

struct RGB
{
    byte r;
    byte g;
    byte b;
};

/* Colors
 */
struct RGB bg_color = {0, 0, 0};
struct RGB clock_text_color = {225, 0, 255};
struct RGB punctuation_color = {255, 255, 255};

/* Variables
*/
bool setup_mode = false;
bool sleep_mode = false;
bool sunrise_mode = false;

byte sleep_timer = 15; // How long to disable sunrise (minutes)
byte alarm_time = 60;  // How long sunrise lasts (minutes)

// TODO:  check if this can be reduced to [7] instead of [7][12]
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
DateTime now_last;
DateTime now_now;

/*
   Notes:
    following DATE(1) format to 'd'
    Sunrise = DATE+1
*/
#define SETTINGS 11
char setting_entries_dict[SETTINGS] = {
    // 'H', 'M', 'S', 'p', 'Y', 'm', 'd', 'u', // Current DateTime
    'H', 'M', 'S', 'p', 'Y', 'm', 'd', 'u', // Current DateTime
    'I', 'N', 'q',                          // Next Sunrise
};
byte setting_entry = 0; // setup mode pointer
byte alarm_setup = 0;   // setup mode pointer for DoW alarm
byte tmp_byte = 0;      // declared globally to prevent repetitive instantiations

/* Program functions
*/

/* EEPROM functions
*/
// Read the byte stored at the target EEPROM address
byte readEByte(byte target)
{
    byte value = EEPROM.read(target);

    return value;
}

// Write the value byte to the target EEPROM address
void writeEByte(byte target, byte value)
{
    EEPROM.write(target, value);
}

// Read the target EEPROM address for sunrise hour
//   target should be the day of the week (0 - 6)
byte getSunriseHour(byte target)
{
    if (target > 6)
    {
        return 0;
    }

    byte value = readEByte(target);

    if (value > 24)
    {
        value = value % 24;
    }
    return value;
}

// Read the target EEPROM address for sunrise minute
//   target should be the day of the week (0 - 6)
byte getSunriseMin(byte target)
{
    if (target > 6)
    {
        return 0;
    }

    // adjust target location for minutes
    target += 7;
    byte value = readEByte(target);

    if (value > 60)
    {
        value = value % 60;
    }
    return value;
}

// Write to the target EEPROM address for sunrise minute
//   target should be the day of the week (0 - 6)
void setSunriseMin(byte target, byte value)
{
    if (target > 6)
    {
        return 0;
    }

    target += 7;
    EEPROM.write(target, value);
}

// Check current time against today's alarm
void checkModeActive()
{
    tmp_byte = now_now.dayOfTheWeek();

    DateTime alarm_dtm = DateTime(
        now_now.year(),
        now_now.month(),
        now_now.day(),
        getSunriseHour(tmp_byte),
        getSunriseMin(tmp_byte),
        0);

    int delta = now_now.unixtime() - alarm_dtm.unixtime();

    if (delta < 0)
    {
        // The sun has not yet risen
        sleep_mode = false;
        sunrise_mode = false;
    }
    else if (delta <= (alarm_time * 60))
    {
        // The sun is rising
        sunrise_mode = (!sleep_mode);
    }
    else
    {
        // The sun will rise again
        sunrise_mode = false;
        sleep_mode = false;
    }
}

/* LED functions
*/
void ledsColor(CRGB color)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = color;
    }

    FastLED.show();
}

// Cycle through all LEDs and set to Black
void turnLedsOff()
{
    ledsColor(CRGB::Black);
}

// Update LEDs for sunrise, brightness varies by time since alarm started
void sunrise()
{
    // Only run this function during sunrise mode (disabled in setup mode)
    if (!sunrise_mode || setup_mode || sleep_mode)
    {
        turnLedsOff();
        return;
    }

    tmp_byte = now_now.dayOfTheWeek();
    int sr_minutes = getSunriseHour(tmp_byte) * 60 + getSunriseMin(tmp_byte);
    int cur_minutes = now_now.hour() * 60 + now_now.minute();

    // Sunrise has ended.
    if (cur_minutes > (sr_minutes + alarm_time))
    {
        sunrise_mode = false;
        sleep_mode = false;
        turnLedsOff();
        return;
    }

    // calculate the current brightness - should start off dim and get brighter towards the end
    int cur_brightness = cur_minutes - sr_minutes;
    FastLED.setBrightness(map(cur_brightness, 0, alarm_time, 5, 255));
    fill_solid(leds, NUM_LEDS, CRGB(255, 150, 25));

    FastLED.show();
}

// Update the states of the LED strand
void updateLed()
{
    if (setup_mode)
    {
        FastLED.setBrightness(25);

        // Flash LEDS to indicate we're in setup mode
        if (now_now.second() % 2 == 0)
        {
            ledsColor(CRGB::DarkSlateGray);
        }
        else
        {
            ledsColor(CRGB::LightGreen);
        }
    }
    else
    {
        sunrise();
    }
}

/* Button functions
*/
byte buttonPress(byte button)
{
    unsigned long start_press = 0;
    byte ret_val = 0;

    while (digitalRead(button) == LOW)
    {
        // initial loop run
        if (start_press == 0)
        {
            start_press = rtc.now().unixtime();
        }

        // Long press
        if ((rtc.now().unixtime() - start_press) >= LONG_BTN_PRESS)
        {
            tft.background(bg_color.r, bg_color.g, bg_color.b);
            ret_val = 2;
            break;
        }
        // short press
        else
        {
            ret_val = 1;
        }
    }

    return ret_val;
}

void increaseSettingsValue()
{
    bool pending_update = false;

    switch (setting_entries_dict[setting_entry])
    {
    case 'Y': // Current year
        rtc.adjust(DateTime(
            now_now.year() + 1,
            now_now.month(),
            now_now.day(),
            now_now.hour(),
            now_now.minute(),
            now_now.second()));
        pending_update = true;
        break;
    case 'm': // Current month
        rtc.adjust(DateTime(
            now_now.year(),
            now_now.month() + 1,
            now_now.day(),
            now_now.hour(),
            now_now.minute(),
            now_now.second()));
        pending_update = true;
        break;
    case 'd': // Current day
        rtc.adjust(DateTime(now_now.unixtime() + 86400));
        pending_update = true;
        break;
    case 'u': // alarm DoW selection
        alarm_setup++;
        if (alarm_setup > 6)
        {
            alarm_setup = 0;
        }
        pending_update = true;
        break;
    case 'H': // Current hour
        rtc.adjust(DateTime(now_now.unixtime() + 3600));
        pending_update = true;
        break;
    case 'M': // Current minute
        rtc.adjust(DateTime(now_now.unixtime() + 60));
        pending_update = true;
        break;
    case 'S': // Current seconds
        rtc.adjust(DateTime(now_now.unixtime() + 2));
        pending_update = true;
        break;
    case 'p': // Current AM/PM
        rtc.adjust(DateTime(now_now.unixtime() + 43200));
        pending_update = true;
        break;
    case 'I': // Sunrise hour
        tmp_byte = getSunriseHour(alarm_setup);
        tmp_byte++;
        writeEByte(alarm_setup, tmp_byte % 24);
        pending_update = true;
        break;
    case 'N': // Sunrise minute
        tmp_byte = getSunriseMin(alarm_setup);
        tmp_byte++;
        setSunriseMin(alarm_setup, tmp_byte % 60);
        pending_update = true;
        break;
    case 'q': // Sunrise am/pm
        tmp_byte = getSunriseHour(alarm_setup);
        tmp_byte = (tmp_byte + 12) % 24;
        writeEByte(alarm_setup, tmp_byte);
        pending_update = true;
        break;
    }

    if (pending_update)
    {
        tft.background(bg_color.r, bg_color.g, bg_color.b);
        now_now = rtc.now();
    }
}

void decreaseSettingsValue()
{
    bool pending_update = false;

    switch (setting_entries_dict[setting_entry])
    {
    case 'Y': // Current year
        rtc.adjust(DateTime(
            now_now.year() - 1,
            now_now.month(),
            now_now.day(),
            now_now.hour(),
            now_now.minute(),
            now_now.second()));
        pending_update = true;
        break;
    case 'm': // Current month
        rtc.adjust(DateTime(
            now_now.year(),
            now_now.month() - 1,
            now_now.day(),
            now_now.hour(),
            now_now.minute(),
            now_now.second()));
        pending_update = true;
        break;
    case 'd': // Current day
        rtc.adjust(DateTime(now_now.unixtime() - 86400));
        pending_update = true;
        break;
    case 'u': // alarm DoW selection
        if (alarm_setup == 0)
        {
            alarm_setup = 6;
        }
        else
        {
            alarm_setup--;
        }
        pending_update = true;
        break;
    case 'H': // Current hour
        rtc.adjust(DateTime(now_now.unixtime() - 3600));
        pending_update = true;
        break;
    case 'M': // Current minute
        rtc.adjust(DateTime(now_now.unixtime() - 60));
        pending_update = true;
        break;
    case 'S': // Current seconds
        rtc.adjust(DateTime(now_now.unixtime() - 1));
        pending_update = true;
        break;
    case 'p': // Current AM/PM
        rtc.adjust(DateTime(now_now.unixtime() - 43200));
        pending_update = true;
        break;
    case 'I': // Sunrise hour
        tmp_byte = getSunriseHour(alarm_setup);
        if (tmp_byte == 0)
        {
            tmp_byte = 24;
        }
        tmp_byte--;
        writeEByte(alarm_setup, tmp_byte);
        pending_update = true;
        break;
    case 'N': // Sunrise minute
        tmp_byte = getSunriseMin(alarm_setup);
        if (tmp_byte == 0)
        {
            tmp_byte = 60;
        }
        tmp_byte--;
        setSunriseMin(alarm_setup, tmp_byte);
        pending_update = true;
        break;
    case 'q': // Sunrise AM/PM
        tmp_byte = getSunriseHour(alarm_setup);
        tmp_byte = (tmp_byte + 12) % 24;
        writeEByte(alarm_setup, tmp_byte);
        pending_update = true;
        break;
    }

    if (pending_update)
    {
        tft.background(bg_color.r, bg_color.g, bg_color.b);
        now_now = rtc.now();
    }
}

void checkBtnOk()
{
    byte results = buttonPress(BTN_OK);

    switch (results)
    {
    case 0: // not pressed
        return;
    case 1: // short press
        if (setup_mode)
        {
            // switch active setting
            setting_entry++;

            // loop back to beginning if we've reached the end
            if (setting_entry == SETTINGS)
            {
                setting_entry = 0;
            }
        }
        else
        { // not in setup mode
            turnLedsOff();
            sunrise_mode = false;
            sleep_mode = true;
        }
        break;
    case 2: // long press
        // toggle setup mode
        setup_mode = !setup_mode;
        // default selected alarm to today
        alarm_setup = now_now.dayOfTheWeek();
        sleep_mode = false;
        tft.background(bg_color.r, bg_color.g, bg_color.b);
        break;
    }
}

void checkBtnLeft()
{
    byte results = buttonPress(BTN_LEFT);

    switch (results)
    {
    case 0: // not pressed
        return;
    case 1: // short press
        if (setup_mode)
        { // in setup mode
            // adjust value
            decreaseSettingsValue();
        }

        break;
    case 2: // long press
        if (setup_mode)
        { // in setup mode
            // switch active setting
            if (setting_entry <= 0)
            {
                // loop around to the end if the beginning is reached
                setting_entry = SETTINGS;
            }

            setting_entry--;
        }
        else
        { // not in setup mode
            turnLedsOff();
        }
        break;
    }
}

void checkBtnRight()
{
    byte results = buttonPress(BTN_RIGHT);

    switch (results)
    {
    case 0: // not pressed
        return;
    case 1: // short press
        if (setup_mode)
        { // in setup mode
            // adjust value
            increaseSettingsValue();
        }

        break;
    case 2: // long press
        if (setup_mode)
        { // in setup mode
            // switch active setting
            setting_entry++;

            if (setting_entry >= SETTINGS)
            {
                // loop around to the beginning if the end is reached
                setting_entry = 0;
            }
        }
        else
        { // not in setup mode
            turnLedsOff();
        }
        break;
    }
}

/* Drawing functions
*/
void tftDrawInfo(byte x, byte y, String value, char setting, bool blank)
{
    tft.stroke(clock_text_color.r, clock_text_color.g, clock_text_color.b);

    // Set the text color in setup mode
    if (setup_mode)
    { // in setup mode
        // Flash the settings entry that is currently selected
        if (setting_entries_dict[setting_entry] == setting)
        {
            if ((now_now.second() % 2) == 0)
            {
                tft.stroke(0, 50, 225);
            }
            else
            {
                tft.stroke(0, 200, 125);
            }
        }
    }

    // blank out a value
    if (blank)
    {
        tft.stroke(bg_color.r, bg_color.g, bg_color.b);
    }

    // create an array the length of the string to write + 1 for null terminate
    char _value_array[value.length() + 1];

    // convert the value to a char array
    value.toCharArray(_value_array, sizeof(_value_array));

    // draw the text at x , y
    tft.text(_value_array, x, y);
}

// Overload for tftDrawInfo(byte, byte, string, char, bool)
void tftDrawInfo(byte x, byte y, int value, char setting, bool blank)
{
    String tmp_str;

    // pad any integer less than 10 with a 0
    if (value < 10)
    {
        tmp_str = "0" + String(value);
    }
    else
    {
        tmp_str = String(value);
    }

    tftDrawInfo(x, y, tmp_str, setting, blank);
}

/* Draw hour's place
      Blank hour if needed
      Write current time to LDC
*/
void drawHour(byte x, byte y)
{
    tmp_byte = now_now.hour() % 12;
    if (tmp_byte == 0)
    {
        tmp_byte = 12;
    }

    // Change hour
    if (now_last.hour() != now_now.hour())
    {
        tmp_byte--;
        if (tmp_byte == 0)
        {
            tmp_byte = 12;
        }

        // Blank the last hour entry from the screen
        tftDrawInfo(x, y, tmp_byte, 'H', true);

        // Revert tmp_byte to current time
        tmp_byte++;
        if (tmp_byte == 13)
        {
            tmp_byte = 1;
        }
    }

    // Draw current hour
    tftDrawInfo(x, y, tmp_byte, 'H', false);
}

/* Draw minute's place
      blank old value
      write new value
 */
void drawMinute(byte x, byte y)
{
    if (now_last.minute() != now_now.minute())
    {
        tftDrawInfo(x, y, now_last.minute(), 'M', true); // Blank minutes - last
    }
    tftDrawInfo(x, y, now_now.minute(), 'M', false); // Draw minutes - current
}

/* Draw second's place
      blank old value
      write new value
 */
void drawSecond(byte x, byte y)
{
    if (now_last.second() != now_now.second())
    {
        tftDrawInfo(x, y, now_last.second(), 'S', true); // Blank second - Latest
    }
    tftDrawInfo(x, y, now_now.second(), 'S', false); // Draw second - current
}

/* Draw AM/PM in place
      blank old value
      write new value
*/
void drawAmPm(byte x, byte y)
{
    if ((now_now.hour() == 12) && (now_last.hour() == 11))
    {
        // blank AM
        tftDrawInfo(x, y, "AM", 'p', true);
    }
    else if ((now_now.hour() == 0) && (now_last.hour() == 23))
    {
        // blank PM
        tftDrawInfo(x, y, "PM", 'p', true);
    }

    if (now_now.hour() < 12)
    {
        // draw AM
        tftDrawInfo(x, y, "AM", 'p', false);
    }
    else
    {
        // draw PM
        tftDrawInfo(x, y, "PM", 'p', false);
    }
}

/* Draw Year's place
      Blank old year
      Write new year
 */
void drawYear(byte x, byte y)
{
    if (now_last.year() != now_now.year())
    {
        tftDrawInfo(x, y, now_last.year(), 'Y', true); // Blank year
    }
    tftDrawInfo(x, y, now_now.year(), 'Y', false); // Draw year
}

/* Draw Month's place
      Blank old month
      Write new month
 */
void drawMonth(byte x, byte y)
{
    if (now_last.month() != now_now.month())
    {
        tftDrawInfo(x, y, now_last.month(), 'm', true); // Blank month
    }
    tftDrawInfo(x, y, now_now.month(), 'm', false); // Draw month
}

/* Draw Day's place
      Blank old day
      Write new day
 */
void drawDay(byte x, byte y)
{
    if (now_last.day() != now_now.day())
    {
        tftDrawInfo(x, y, now_last.day(), 'd', true); // Blank day
    }
    tftDrawInfo(x, y, now_now.day(), 'd', false); // Draw day
}

/* Draw Day of the Week
      Blank old day of the week
      Write new day of the week
 */
void drawDoW(byte x, byte y)
{
    x = 15;
    for (int i = 0; i < 7; i++)
    {
        if (now_now.dayOfTheWeek() != now_last.dayOfTheWeek())
        {
            // blank last DoW box
            tft.stroke(bg_color.r, bg_color.g, bg_color.b);
            tft.rect(x - 2, y - 2, 9, 11);
        }

        if (now_now.dayOfTheWeek() == i)
        {
            // Draw a box around the DoW letter
            //   implementation note:  drawing lines results in less screen
            //   flicker than drawing a rect.
            tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
            tft.line(x - 2, y - 2, x + 6, y - 2);
            tft.line(x - 2, y - 2, x - 2, y + 8);
            tft.line(x + 6, y - 2, x + 6, y + 9);
            tft.line(x - 2, y + 8, x + 6, y + 8);
            // tft.rect(x - 2, y - 2, 9, 11);
        }

        if (setup_mode && (alarm_setup == i))
        {
            tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
            tft.line(x - 2, y + 10, x + 6, y + 10);
        }

        // Draw the DoW letter
        tftDrawInfo(x, y, String(daysOfTheWeek[i][0]), 'u', false);

        // Increment the cursor
        x += 20;
    }
}

void drawNextSunrise(byte x, byte y)
{
    if (setup_mode)
    {
        // Display alarm time for selected day
        byte sr_hour = getSunriseHour(alarm_setup);
        byte sr_min = getSunriseMin(alarm_setup);

        tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
        tft.text("Next Sunrise:", x, y);
        x = 25;
        y += 10;

        tmp_byte = sr_hour % 12;
        if (tmp_byte == 0)
        {
            tmp_byte = 12;
        }
        tftDrawInfo(x, y, tmp_byte, 'I', false);
        x += 12;
        tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
        tft.text(":", x, y);
        x += 5;
        tftDrawInfo(x, y, sr_min, 'N', false);
        x += 15;
        if (sr_hour >= 12)
        {
            tftDrawInfo(x, y, "PM", 'q', false);
        }
        else
        {
            tftDrawInfo(x, y, "AM", 'q', false);
        }

        // 23,400 == 6:30 == 6 * 3600 + 30 * 60
        // 23,400 / 3600 == 6.5
        // 23,400 - (6 * 3600) / 60 == 30

        return;
    }

    // update this field once per minute when out of setup mode
    if (now_now.minute() != now_last.minute())
    {
        tmp_byte = now_now.dayOfTheWeek();
        int sr_minutes = getSunriseHour(tmp_byte) * 60 + getSunriseMin(tmp_byte);
        int cur_minutes = now_now.hour() * 60 + now_now.minute();

        String message = "";

        if (cur_minutes < sr_minutes)
        {
            // Current time is before sunrise
            tmp_byte = (sr_minutes - cur_minutes) / 60;
            if (tmp_byte == 0)
            {
                message = "Sunrise in less than an hour";
            }
            else if (tmp_byte == 1)
            {
                message = "Sunrise in an hour";
            }
            else
            {
                message = "Sunrise in " + String(tmp_byte) + " hours";
            }
        }
        else if ((cur_minutes - sr_minutes) <= alarm_time)
        {
            // Current time is during sunrise
            message = "The sun is rising";
        }
        else
        {
            // The sun has risen
            message = "Sunrise is tomorrow";
        }

        // Blank previous message
        tft.stroke(bg_color.r, bg_color.g, bg_color.b);
        tft.fill(bg_color.r, bg_color.g, bg_color.b);
        tft.rect(x, y, tft.width(), 7);

        // Write new message
        tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
        tftDrawInfo(x, y, message, 'I', false);
    }
}

void drawCurrentMode(byte x, byte y)
{
    // Update every 15 seconds
    if ((now_now.second() % 15 == 0) && now_now.second() != now_last.second())
    {

        String message = "Current mode:  ";

        if (setup_mode)
        {
            message += "Setup";
        }
        else if (sleep_mode)
        {
            message += "Sleeping";
        }
        else if (sunrise_mode)
        {
            message += "Sunrise";
        }
        else
        {
            message += "Clock";
        }

        tft.stroke(bg_color.r, bg_color.g, bg_color.b);
        tft.fill(bg_color.r, bg_color.g, bg_color.b);
        tft.rect(x, y, tft.width(), 10);
        tftDrawInfo(x, y, message, 'Z', false);
    }
}

// Update information displayed on LCD
void updateLcd()
{
    byte x = 5;
    byte y = 0;

    /****************************** line 1 ******************************/
    tft.setTextSize(3);
    drawHour(x, y);

    x = 35;
    tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
    tft.text(":", x, y);

    x = 47;
    drawMinute(x, y);

    x = 77;
    tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
    tft.text(":", x, y);

    x = 89;
    drawSecond(x, y);

    tft.setTextSize(2);
    x = 130;
    y = 5;
    drawAmPm(x, y);

    /****************************** line 2 ******************************/
    tft.setTextSize(2);

    // Current Year
    x = 10;
    y = 35;
    drawYear(x, y);

    x = 58;
    tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
    tft.text("/", x, y);

    // Current Month
    x = 70;
    drawMonth(x, y);

    x = 95;
    tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
    tft.text("/", x, y);

    // Current Day
    x = 108;
    drawDay(x, y);

    /****************************** line 3 ******************************/
    tft.setTextSize(1);
    // Current Day of the Week
    x = 50;
    y = 60;
    drawDoW(x, y);

    /****************************** line 4 ******************************/
    tft.setTextSize(1);
    tft.stroke(255, 225, 0);

    x = 0;
    y = 90;
    drawNextSunrise(x, y);

    x = 65;

    /****************************** line 5 ******************************/
    x = 0;
    y = 120;
    drawCurrentMode(x, y);

    // /****************************** line 6 ******************************/
    // x = 80;
    // y = 120;
}
