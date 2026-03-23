
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <HWCDC.h>

#include <lvgl.h>

#include "BISystemMessage.h"
#include "BIIOExpander.h"

#include "BIDataStore.h"
#include "BITouch.h"
#include "BIDisplay.h"
#include "BIWiFi.h"
#include "BIWebServer.h"
#include "BITimeKeeping.h"
#include "BISignalK.h"

#define ARDUHAL_LOG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE

HWCDC USBSerial;

void usbSerialLogger(const system_message_level_t level, const char *buffer) {
/*  if (!level == OFF) {
    USBSerial.print(asStr(level)); 
  }
*/  USBSerial.print(buffer);
//  USBSerial.flush();
}

void initUSBSerial(unsigned long baudrate, unsigned long timeout_ms) {
  // Serial
  USBSerial.begin(baudrate);
 
  unsigned long start_time = millis();
    
  // Warte auf Verbindung, aber maximal bis zum Timeout
  while (!USBSerial && (millis() - start_time < timeout_ms)) {
    delay(10);
  }

  if (USBSerial) {
    biRegisterSystemMessageConsumer(usbSerialLogger);
  }
}


void setup() {
  // 1. Initialization of the IOExpander
  // This is a two step initialization because there are some initialization task to be done
  // really quick after startup.
  // - set initial values of ports
  // - initialize IO direction of ports
  // - silencing the buzzer
  initIOExpander();

  // 2. Initialize touch (BEFORE display)
  initTouch();
  
  // 3. Set backlight
  setBackLightPercent(30); // ~50% brightness

  // 4. Final IOExpander initialization tasks
  // This is the second stage of initialization
  // - set values of ports
  // - set final IO direction of ports
  initIOExpanderFinalPath();

  // 5. Initialize System Message Queue
  initSystemMessage();

  // A short Beep, the IOExpander is up and running and well configured,
  // power is locked, key 1 may be released
//  beep(100);
 
  initUSBSerial(115200, 500);

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "---- Amal BoatInstrument is starting ---\n");

  // 6. Load stored configuration data from LVRam
  initDataStore();

  // 7. Initialize Display Driver
  initDisplay();

  // 8. Initialize WiFi
  // According to stored configuration setup as AccessPoint or connecting to one
  initWiFi();

  // 9. Initialize Clock
  initClock();

  // 10. Initialize HTTP Server
  initHTTPServer();

  // 11. Initialize SignalK
  initSignalK();

  // Now, that everything is configured and LV is up and running, remove GFX from SystemMessageConsumerList.
  // SystemMessages will now be listed in LoggingScreen
  biRemoveGFXSystemMessageLogger();
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

/*    if (getBatteryVoltage(&batVoltage) && getChargerStatus(&chargerStatus)) {
      biEnqueueSystemMessage(INFO, "Battery %dV %s.\n", batVoltage, chargerStatus == 0 ? "Charging" : "Idle");
    } else {
      biEnqueueSystemMessage(WARN, "Getting Battery Voltage failed.\n");
    }
*/
  }

}
