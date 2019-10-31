/* Sunrise alarm clock
   By: Tim Forbes
   Latest update: October 30, 2019
*/
#include "./arduino_sunrise.h"

/* Script Start
*/
void setup()
{
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
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    tft.stroke(255, 0, 0);
    tft.text("RTC not found!", 0, 0);
    while (1)
      ;
  }

  // Set the RTC if necessary
  if (!rtc.isrunning())
  {
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

void loop()
{
  now_now = rtc.now();

  // Check if any buttons are being pressed
  checkBtnOk();
  checkBtnLeft();
  checkBtnRight();

  // Sunrise currently active, sleeping, or off?
  checkModeActive();

  // Update LED
  updateLed();

  // Update LCD
  updateLcd();

  now_last = now_now;
}
