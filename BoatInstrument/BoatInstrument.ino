
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <HWCDC.h>

#include <Arduino_GFX_Library.h>
#include <lvgl.h>

#include "BISystemMessage.h"
#include "BIIOExpander.h"

#include "BIDataStore.h"
#include "BITouch.h"
#include "BIDisplay.h"
#include "BIWiFi.h"
#include "BIWebServer.h"
//#include "BITimeKeeping.h"
#include "BISignalK.h"

extern Arduino_RGB_Display *gfx;
HWCDC USBSerial;


//TouchDrvGT911 GT911;
int16_t x[5], y[5];
uint8_t gt911_i2c_addr = 0;

void usbSerialLogger(const system_message_level_t level, const char *buffer) {
  if (!level == OFF) {
    USBSerial.print(asStr(level)); 
  }
  USBSerial.print(buffer);
  USBSerial.flush();
}

void initUSBSerial(unsigned long baudrate, unsigned long timeout_ms) {
  // Serial
  USBSerial.begin(baudrate);
 
  unsigned long start_time = millis();
    
  // Warte auf Verbindung, aber maximal bis zum Timeout
  while (!USBSerial && (millis() - start_time < timeout_ms)) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (USBSerial) {
    biRegisterSystemMessageConsumer(usbSerialLogger);
  }
}


void setup() {
  // 1. Initialisation
  initIOExpander();

  // 2. Initialize touch (BEFORE display)
  initTouch();
  
  // 3. Set backlight
  setBackLightPercent(70); // ~70% brightness

  // 4. 5. Final initialization tasks
  initIOExpanderFinalPath();

  biInitSystemMessage();

  // A short Beep, the IOExpander is up and running and well configured.
  // Power is locked, key 1 may be released
  beep(100);
 
  initUSBSerial(115200, 5000);

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "---- Amal BoatInstrument is starting ---\n");

  // 6. Load stored configuration data
  initDataStore();

  // 7. Initialize Display
  initDisplay();

  // 8. Initialize WiFi
  initWiFi();

  // 9. Initialize Clock
//  initClock();

  // 10. Initialize HTTP Server
  initHTTPServer();

  // 11. Initialize SignalK
  initSignalK();
}

unsigned long startZeit = 0;

void loop() {

  lv_timer_handler(); /* let the GUI do its work */

  // Small delay to prevent excessive loop iterations
  vTaskDelay(pdMS_TO_TICKS(1)); 
  yield();

  if (millis() - startZeit >= 5000) {
    startZeit = millis();

    uint16_t batVoltage = 0;
    uint8_t chargerStatus = 0;

    if (getBatteryVoltage(&batVoltage) && getChargerStatus(&chargerStatus)) {
      biEnqueueSystemMessage(INFO, "Battery %dV %s.\n", batVoltage, chargerStatus == 0 ? "Charging" : "Idle");
    } else {
      biEnqueueSystemMessage(WARN, "Getting Battery Voltage failed.\n");
    }
  }
}
