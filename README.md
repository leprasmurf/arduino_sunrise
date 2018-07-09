# Sunrise alarm clock

## Hardware
* Arduino pro-mini (5v 16mhz)
* Alitove [16.4ft WS2812B](https://amzn.to/2L266wj) LED strip <- (amazon affiliate link to the LED strip I used)
* Alitove [5v 10a Power brick](https://amzn.to/2zoA7Vr) (amazon affiliate link again)
* [TFT 1.8 display](https://www.adafruit.com/product/358)
* [ds1307 RTC](https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit)
* [Generic Project Box](https://amzn.to/2L0KvnS) (another affiliate link.  Could accomplish the same with a 3d printed version)

## Connections
* LED **Data** <-> **D6** Arduino
* RTC **SDA** <-> **A4** Arduino
* RTC **SCL** <-> **A5** Arduino
* Screen **D/C** <-> **D8** Arduino
* Screen **RST** <-> **D9** Arduino
* Screen **CS** <-> **D10** Arduino
* Screen **MOSI** <-> **D11** Arduino
* Screen **SCK** <-> **D13** Arduino
* Screen SD **CS** <-> **D4** Arduino
* Screen SD **MISO** <-> **D12** Arduino

## External Libraries
* Adafruit [RTCLib](https://github.com/adafruit/RTClib)
* [FastLED](https://github.com/FastLED/FastLED)

## Arduino Libraries
* [SPI](https://www.arduino.cc/en/Reference/SPI)
* [TFT](https://www.arduino.cc/en/Reference/TFTLibrary)
* [Wire](https://www.arduino.cc/en/Reference/Wire)

## Operation
One hour prior to the target the LED strip will illuminate (full intensity for sunset, minimal for sunrise).  Over the next hour the brightness is inverted on a linear scale.  The RTC has a battery backup and can maintain the date/time displayed.  The target time for sunrise and sunset is stored in the [Arduino's EEPROM](https://www.arduino.cc/en/Reference/EEPROM).


Each of the three momentary button has 2x available actions:  short and long press.  Long pressing the middle (OK) button toggles setup mode.  While in setup mode (indicated by the flashing LED strip) each value can be adjusted.  The currently selected value is highlighted and can be adjusted by the left or right buttons.  Short pressing left or right will adjust the selected value down or up respectively, whereas long pressing left or right will change which setting is currently selected.  Short press OK to advance to the next setting as well.

## Roadmap
* Finish cleaning code (move support functions into header file)
* Add Sugru over hot-snot holding screen in place (create a smooth bevel).
* Add Sugru over the momentary switches.
* Enhance sunrise and sunset routines to transition between two colors.
* Add buzzer for simple notifications.
* Add support for the SD Card reader on the screen; screensaver with timestamp
