/* Sunrise alarm clock
   By Tim
   Latest update: July 9, 2018
*/
#include "arduino_sunrise.h"

/* global functions
*/
// return the address of each target
byte modeAddress(String target) {
  if (target == "sunrise_hour") {
    return 0;
  } else if (target == "sunrise_min") {
    return 1;
  } else if (target == "sunset_hour") {
    return 2;
  } else if (target == "sunset_min") {
    return 3;
  }
}

// Read the EEPROM byte at the address associated with target
byte readEByte(String target) {
  // Translate the memory target to a byte address
  byte address = modeAddress(target);

  // Read the memory location
  byte value = EEPROM.read(address);

  return value;
}

// Read the EEPROM byte at the address associated with target
byte writeEByte(String target, byte value) {
  // Translate the memory target to a byte address
  byte address = modeAddress(target);

  // Write the passed byte value to address
  // (only if different from what's already there)
  EEPROM.update(address, value);
}

/* LED functions
*/
void ledsColor(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }

  FastLED.show();
}

// Cycle through all LEDs and set to Black
void turnLedsOff() {
  ledsColor(CRGB::Black);
}

void sunrise() {
  // Only run this function during sunrise mode (disabled in setup mode)
  if (!sunrise_mode || setup_mode || sunset_mode) {
    return;
  }
  Serial.println("Sunrise mode");

  int abs_cur = now_now.hour() * 60 + now_now.minute();
  int abs_sr = readEByte("sunrise_hour") * 60 + readEByte("sunrise_min");

  tmp_byte = byte(abs_sr - abs_cur);

  int brightness = (TARGET_MIN - tmp_byte) * 100 / TARGET_MIN;

  if (tmp_byte > TARGET_MIN) {
    Serial.print("Too much time until sunrise target:  ");
    Serial.println(tmp_byte);
    sunrise_mode = false;
    turnLedsOff();
    return;
  }

  FastLED.setBrightness( constrain(byte(brightness * 2.55), 5, 255) );
  fill_solid(leds, NUM_LEDS, CRGB(255, 255, 0));

  FastLED.show();
}

void sunset() {
  // Only run this function during sunset mode (disabled in setup mode)
  if (!sunset_mode || setup_mode || sunrise_mode) {
    return;
  }
  Serial.println("Sunset mode");
  
  int abs_cur = now_now.hour() * 60 + now_now.minute();
  int abs_ss = readEByte("sunset_hour") * 60 + readEByte("sunset_min");

  tmp_byte = byte(abs_ss - abs_cur);

  int brightness = tmp_byte * 100 / TARGET_MIN;
  
  if (tmp_byte > TARGET_MIN) {
    Serial.print("Too much time until sunset target:  ");
    Serial.println(tmp_byte);
    sunset_mode = false;
    turnLedsOff();
    return;
  }

  FastLED.setBrightness( constrain(byte(brightness * 2.55), 5, 255) );
  fill_solid(leds, NUM_LEDS, CRGB(125, 100, 255));

  FastLED.show();
}

// Update the states of the LED strand
void updateLed() {
  if (setup_mode) {
    FastLED.setBrightness( 25 );

    // Flash LEDS to indicate we're in setup mode
    if (now_now.second() % 2 == 0) {
      ledsColor(CRGB::DarkSlateGray);
    } else {
      ledsColor(CRGB::LightGreen);
    }
  } else {
    sunrise();
    sunset();
  }
}

/* Button functions
*/
byte buttonPress(byte button) {
  unsigned long start_press = 0;
  byte ret_val = 0;

  while (digitalRead(button) == LOW) {
    // initial loop run
    if (start_press == 0) {
      start_press = rtc.now().unixtime();
      Serial.print("Button pressed at ");
      Serial.println(start_press);
    }

    // Long press
    if ((rtc.now().unixtime() - start_press) >= LONG_BTN_PRS) {
      tft.background(bg_color.r, bg_color.g, bg_color.b);
      ret_val = 2;
    } else { // short press
      ret_val = 1;
    }
  }

  return ret_val;
}

void increaseSettingsValue() {
  bool pending_update = false;

  switch (setting_entries_dict[setting_entry]) {
    case 'Y': // Current year
      rtc.adjust(DateTime(
                   now_now.year() + 1,
                   now_now.month(),
                   now_now.day(),
                   now_now.hour(),
                   now_now.minute(),
                   now_now.second()
                 ));
      pending_update = true;
      break;
    case 'm': // Current month
      rtc.adjust(DateTime(
                   now_now.year(),
                   now_now.month() + 1,
                   now_now.day(),
                   now_now.hour(),
                   now_now.minute(),
                   now_now.second()
                 ));
      pending_update = true;
      break;
    case 'd': // Current day
      rtc.adjust(DateTime(now_now.unixtime() + 86400));
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
    case 'u': // Day of the Week
      rtc.adjust(DateTime(now_now.unixtime() + 86400));
      pending_update = true;
      break;
    case 'I': // Sunrise hour
      tmp_byte = readEByte("sunrise_hour");
      tmp_byte++;
      writeEByte("sunrise_hour", tmp_byte % 24);
      pending_update = true;
      break;
    case 'N': // Sunrise minute
      tmp_byte = readEByte("sunrise_min");
      tmp_byte++;
      writeEByte("sunrise_min", tmp_byte % 60);
      pending_update = true;
      break;
    case 'q': // Sunrise am/pm
      tmp_byte = readEByte("sunrise_hour");
      tmp_byte = (tmp_byte + 12) % 24;
      writeEByte("sunrise_hour", tmp_byte);
      pending_update = true;
      break;
    case 'J': // Sunset hour
      tmp_byte = readEByte("sunset_hour");
      tmp_byte++;
      writeEByte("sunset_hour", tmp_byte % 24);
      pending_update = true;
      break;
    case 'O': // Sunset minute
      tmp_byte = readEByte("sunset_min");
      tmp_byte++;
      writeEByte("sunset_min", tmp_byte % 60);
      pending_update = true;
      break;
    case 'r': // Sunset am/pm
      tmp_byte = readEByte("sunset_hour");
      tmp_byte = (tmp_byte + 12) % 24;
      writeEByte("sunset_hour", tmp_byte);
      pending_update = true;
      break;
  }

  if (pending_update) {
    tft.background(bg_color.r, bg_color.g, bg_color.b);
    now_now = rtc.now();
  }
}

void decreaseSettingsValue() {
  bool pending_update = false;

  switch (setting_entries_dict[setting_entry]) {
    case 'Y': // Current year
      rtc.adjust(DateTime(
                   now_now.year() - 1,
                   now_now.month(),
                   now_now.day(),
                   now_now.hour(),
                   now_now.minute(),
                   now_now.second()
                 ));
      pending_update = true;
      break;
    case 'm': // Current month
      rtc.adjust(DateTime(
                   now_now.year(),
                   now_now.month() - 1,
                   now_now.day(),
                   now_now.hour(),
                   now_now.minute(),
                   now_now.second()
                 ));
      pending_update = true;
      break;
    case 'd': // Current day
      rtc.adjust(DateTime(now_now.unixtime() - 86400));
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
    case 'u': // Day of the Week
      rtc.adjust(DateTime(now_now.unixtime() - 86400));
      pending_update = true;
      break;
    case 'I': // Sunrise hour
      tmp_byte = readEByte("sunrise_hour");
      if (tmp_byte == 0) {
        tmp_byte = 24;
      }
      tmp_byte--;
      writeEByte("sunrise_hour", tmp_byte);
      pending_update = true;
      break;
    case 'N': // Sunrise minute
      tmp_byte = readEByte("sunrise_min");
      if (tmp_byte == 0) {
        tmp_byte = 60;
      }
      tmp_byte--;
      writeEByte("sunrise_min", tmp_byte);
      pending_update = true;
      break;
    case 'q': // Sunrise am/pm
      tmp_byte = readEByte("sunrise_hour");
      tmp_byte = (tmp_byte + 12) % 24;
      writeEByte("sunrise_hour", tmp_byte);
      pending_update = true;
      break;
    case 'J': // Sunset hour
      tmp_byte = readEByte("sunset_hour");
      if (tmp_byte == 0) {
        tmp_byte = 24;
      }
      tmp_byte--;
      writeEByte("sunset_hour", tmp_byte);
      pending_update = true;
      break;
    case 'O': // Sunset minute
      tmp_byte = readEByte("sunset_min");
      if (tmp_byte == 0) {
        tmp_byte = 60;
      }
      tmp_byte--;
      writeEByte("sunset_min", tmp_byte);
      pending_update = true;
      break;
    case 'r': // Sunset am/pm
      tmp_byte = readEByte("sunset_hour");
      tmp_byte = (tmp_byte + 12) % 24;
      writeEByte("sunset_hour", tmp_byte);
      pending_update = true;
      break;

  }

  if (pending_update) {
    tft.background(bg_color.r, bg_color.g, bg_color.b);
    now_now = rtc.now();
  }
}

void checkBtnOk() {
  byte results = buttonPress(BTN_OK);

  switch (results) {
    case 0: // not pressed
      return;
    case 1: // short press
      if (setup_mode) { // in setup mode
        // switch active setting
        setting_entry++;

        if (setting_entry == SETTINGS) {
          setting_entry = 0;
        }
      } else { // not in setup mode
        turnLedsOff();
        sunrise_mode = false;
        sunset_mode = false;
      }
      Serial.println("OK short press");
      break;
    case 2: // long press
      Serial.println("OK long press");
      // toggle setup mode
      setup_mode = !setup_mode;
      tft.background(bg_color.r, bg_color.g, bg_color.b);
      break;
  }
}

void checkBtnLeft() {
  byte results = buttonPress(BTN_LEFT);

  switch (results) {
    case 0: // not pressed
      return;
    case 1: // short press
      Serial.println("Left short press");
      if (setup_mode) { // in setup mode
        // adjust value
        decreaseSettingsValue();
      }

      break;
    case 2: // long press
      Serial.println("Left long press");
      if (setup_mode) { // in setup mode
        // switch active setting
        if (setting_entry == 0) {
          setting_entry = SETTINGS;
        }

        setting_entry--;
      } else { // not in setup mode
        turnLedsOff();
      }
      break;
  }
}

void checkBtnRight() {
  byte results = buttonPress(BTN_RIGHT);

  switch (results) {
    case 0: // not pressed
      return;
    case 1: // short press
      Serial.println("Right short press");
      if (setup_mode) { // in setup mode
        // adjust value
        increaseSettingsValue();
      }

      break;
    case 2: // long press
      Serial.println("Right long press");
      if (setup_mode) { // in setup mode
        // switch active setting
        setting_entry++;

        if (setting_entry == SETTINGS) {
          setting_entry = 0;
        }
      } else { // not in setup mode
        turnLedsOff();
      }
      break;
  }
}

// Check current time against alarm times
void checkModeActive() {
  int abs_cur = now_now.hour() * 60 + now_now.minute();
  int abs_sr = readEByte("sunrise_hour") * 60 + readEByte("sunrise_min");
  int abs_ss = readEByte("sunset_hour") * 60 + readEByte("sunset_min");

  if(((abs_sr - abs_cur) >= 0) && ((abs_sr - abs_cur) <= TARGET_MIN)) {
    sunrise_mode = true;
    sunset_mode = false;
  }

  if(((abs_ss - abs_cur) >= 0) && ((abs_ss - abs_cur) <= TARGET_MIN)) {
    sunrise_mode = false;
    sunset_mode = true;
  }
}

/* LCD functions
*/
void tftDrawInfo(byte x, byte y, String value, char setting, bool blank) {
  tft.stroke(clock_text_color.r, clock_text_color.g, clock_text_color.b);

  // Set the text color in setup mode
  if (setup_mode) { // in setup mode
    // Flash the settings entry that is currently selected
    if (setting_entries_dict[setting_entry] == setting) {
      if ((now_now.second() % 2) == 0) {
        tft.stroke(0, 50, 225);
      } else {
        tft.stroke(0, 200, 125);
      }
    }
  }

  // blank out a value
  if (blank) {
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
void tftDrawInfo(byte x, byte y, int value, char setting, bool blank) {
  String tmp_str;

  // pad any integer less than 10 with a 0
  if (value < 10) {
    tmp_str = "0" + String(value);
  } else {
    tmp_str = String(value);
  }

  tftDrawInfo(x, y, tmp_str, setting, blank);
}

/* Change hour
      Blank last hour from screen
      Write new value
*/
void checkHour(byte x, byte y) {
  if (now_last.hour() != now_now.hour()) {
    // Get the last hour entry
    tmp_byte = now_last.hour() % 12;
    if (tmp_byte == 0) {
      tmp_byte = 12;
    }

    // Blank the last hour entry from the screen
    tftDrawInfo(x, y, tmp_byte, 'H', true); // Blank hours - last

    // Get the current hour entry
    tmp_byte = now_now.hour() % 12;
    if (tmp_byte == 0) {
      tmp_byte = 12;
    }

    // Draw the current hour
    tftDrawInfo(x, y, tmp_byte, 'H', false); // Draw hours - current
  }
}

/* Change minute
      blank old value
      write new value
 */
void checkMinute(byte x, byte y) {
  if(now_last.minute() != now_now.minute()) {
    tftDrawInfo(x, y, now_last.minute(), 'M', true); // Blank minutes - last
    tftDrawInfo(x, y, now_now.minute(), 'M', false); // Draw minutes - current
  }
}

/* Change seconds
      blank old value
      write new value
 */
void checkSecond(byte x, byte y) {
  if(now_last.second() != now_now.second()) {
    tftDrawInfo(x, y, now_last.second(), 'M', true); // Blank second - last
    tftDrawInfo(x, y, now_now.second(), 'M', false); // Draw second - current
  }
}

/* Change AM/PM
      blank old value
      write new value
*/
void checkAmPm(byte x, byte y) {
  if ((now_now.hour() == 12) && (now_last.hour() == 11)) {
    tftDrawInfo(x, y, "AM", 'p', true);
    tftDrawInfo(x, y, "PM", 'p', false);
  } else if ((now_now.hour() == 0) && (now_last.hour() == 23)) {
    tftDrawInfo(x, y, "PM", 'p', true);
    tftDrawInfo(x, y, "AM", 'p', false);
  }
}

/* Change Year
      Blank old year
      Write new year
 */
void checkYear(byte x, byte y) {
  if (now_last.year() != now_now.year()) {
    tftDrawInfo(x, y, now_last.year(), 'Y', true); // Blank year
    tftDrawInfo(x, y, now_now.year(), 'Y', false); // Draw year
  }
}

/* Change Month
      Blank old month
      Write new month
 */
void checkMonth(byte x, byte y) {
  if (now_last.month() != now_now.month()) {
    tftDrawInfo(x, y, now_last.month(), 'm', true); // Blank month
    tftDrawInfo(x, y, now_now.month(), 'm', false); // Draw month
  }
}

/* Change Day
      Blank old day
      Write new day
 */
void checkDay(byte x, byte y) {
  if (now_last.day() != now_now.day()) {
    tftDrawInfo(x, y, now_last.day(), 'd', true); // Blank day
    tftDrawInfo(x, y, now_now.day(), 'd', false); // Draw day
  }
}

/* Change Day of the Week
      Blank old day of the week
      Write new day of the week
 */
void checkDoW(byte x, byte y) {
  if (now_last.dayOfTheWeek() != now_now.dayOfTheWeek()) {
    // blank DoW line
    // tft.stroke(clock_text_color.r, clock_text_color.g, clock_text_color.b);

    tftDrawInfo(x, y, String(daysOfTheWeek[now_last.dayOfTheWeek()]), 'u', true); // Blank DoW
    tftDrawInfo(x, y, String(daysOfTheWeek[now_now.dayOfTheWeek()]), 'u', true); // Draw DoW
  }
}

// Update information displayed on LCD
void updateLcd() {
  byte x = 5;
  byte y = 0;

  /****************************** line 1 ******************************/
  tft.setTextSize(3);
  checkHour(x, y);

  x = 35;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text(":", x, y);

  x = 47;
  checkMinute(x, y);
  
  x = 77;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text(":", x, y);

  x = 89;
  checkSecond(x, y);
  
  tft.setTextSize(2);
  x = 130;
  y = 5;
  checkAmPm(x, y);

  /****************************** line 2 ******************************/
  tft.setTextSize(2);

  // Current Year
  x = 10;
  y = 35;
  checkYear(x, y);


  x = 58;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text("/", x, y);

  // Current Month
  x = 70;
  checkMonth(x, y);
  
  x = 95;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text("/", x, y);

  // Current Day
  x = 108;
  checkDay(x, y)

  /****************************** line 3 ******************************/
  tft.setTextSize(1);
  // Current Day of the Week
  x = 50;
  y = 60;
  checkDoW(x, y);

  /****************************** line 4 ******************************/
  tft.setTextSize(1);
  tft.stroke(255, 225, 0);

  x = 0;
  y = 90;
  tft.text("Sunrise", 0, y);

  x = 45;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text("@", x, y);

  // Draw AM/PM before the tmp_byte is manipulated from original value
  x = 85;
  tmp_byte = constrain(readEByte("sunrise_hour"), 0, 23);
  if (tmp_byte >= 12) {
    tftDrawInfo(x, y, "PM", 'q', false); // Draw PM - Sunrise
  } else {
    tftDrawInfo(x, y, "AM", 'q', false); // Draw AM - Sunrise
  }

  x = 55;
  tmp_byte = tmp_byte % 12;
  if (tmp_byte == 0) {
    tmp_byte = 12; // 00:00 should read 12:00 AM
  }
  tftDrawInfo(x, y, tmp_byte, 'I', false); // Draw Hour - Sunrise

  x = 65;
  tft.text(":", x, y);

  x = 70;
  tmp_byte = constrain(readEByte("sunrise_min"), 0, 59);
  tftDrawInfo(x, y, tmp_byte, 'N', false); // Draw Minute - Sunrise

  /****************************** line 5 ******************************/
  tft.setTextSize(1);
  tft.stroke(75, 25, 255);

  x = 0;
  y = 100;
  tft.text("Sunset", x, y);

  x = 45;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text("@", x, y);

  // Draw AM/PM before the tmp_byte is manipulated from original value
  x = 85;
  tmp_byte = constrain(readEByte("sunset_hour"), 0, 23);
  if (tmp_byte >= 12) {
    tftDrawInfo(x, y, "PM", 'r', false); // Draw PM - Sunset
  } else {
    tftDrawInfo(x, y, "AM", 'r', false); // Draw AM - Sunset
  }

  x = 55;
  tmp_byte = tmp_byte % 12;
  if (tmp_byte == 0) {
    tmp_byte = 12; // 00:00 should read 12:00 AM
  }
  tftDrawInfo(x, y, tmp_byte, 'J', false); // Draw Hour - Sunset

  x = 65;
  tft.text(":", x, y);

  x = 70;
  tmp_byte = constrain(readEByte("sunset_min"), 0, 59);
  tftDrawInfo(x, y, tmp_byte, 'O', false); // Draw Minute - Sunset


  /****************************** line 6 ******************************/
  x = 80;
  y = 120;
  tft.stroke(punctuation_color.r, punctuation_color.g, punctuation_color.b);
  tft.text("Current mode:", 0, 120);

  // Current mode
  tft.stroke(bg_color.r, bg_color.g, bg_color.b);
  tft.fill(bg_color.r, bg_color.g, bg_color.b);
  tft.rect(x, y, (tft.width() - x), (tft.height() - y));

  if (sunrise_mode) {
    tftDrawInfo(x, y, "Sunrise", '_', false);
  } else if (sunset_mode) {
    tftDrawInfo(x, y, "Sunset", '_', false);
  } else if (setup_mode) {
    tftDrawInfo(x, y, "Setup", '_', false);
  }
}

/* Script Start
*/
void setup() {
  Serial.begin(115200);
  Serial.println("Clock startup");

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  tft.begin();
  tft.setRotation(3);
  tft.background(bg_color.r, bg_color.g, bg_color.b);

  // Define the LED strip driver and color calibration
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  // Button Mode change
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  // Verify RTC exists
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    tft.stroke(255, 0, 0);
    tft.text("RTC not found!", 0, 0);
    while (1);
  }

  // Set the RTC if necessary
  if (! rtc.isrunning()) {
    Serial.println("RTC not running!  Setting to last compile DateTime: " + String(F(__DATE__)) + " at " + String(F(__TIME__)));

    // set the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("Setup complete!");
  Serial.print("Current RTC Date/Time: ");
  Serial.print(now_now.year());
  Serial.print("/");
  Serial.print(now_now.month());
  Serial.print("/");
  Serial.print(now_now.day());
  Serial.print(" @ ");
  Serial.print(now_now.hour());
  Serial.print(":");
  Serial.print(now_now.minute());
  Serial.print(":");
  Serial.println(now_now.second());

  Serial.print("Unix time: ");
  Serial.println(now_now.unixtime());
}

void loop() {
  now_now = rtc.now();

  // Check if any buttons are being pressed
  checkBtnOk();
  checkBtnLeft();
  checkBtnRight();

  checkModeActive();

  // Update LED
  updateLed();

  // Update LCD
  updateLcd();

  now_last = now_now;
}
