#include <Arduino.h>
#include <WiFi.h>
//#include <NetworkClientSecure.h>  -  Vielleicht ein Weg, um SSL zu verwenden ?!

#include <ArduinoHttpClient.h>

#include "BISystemMessage.h"
#include "BIDataStore.h"


static bool signalkEnabled = false;
static TaskHandle_t signalKTaskHandle = NULL;
WiFiClient wifiClient;
//NetworkClientSecure clientSecure;  -  Vielleicht ein Weg, um SSL zu verwenden ?!

// Task function to handle reboot
void signalKTask(void *pvParameters) {
    WebSocketClient* webSocketClient;

    webSocketClient = new WebSocketClient(wifiClient, getSignalKIP().c_str(), getSignalKPort());
    webSocketClient->begin("/signalk/v1/stream");

    while (signalkEnabled && webSocketClient->connected()) {
      webSocketClient->beginMessage(TYPE_TEXT);

      webSocketClient->endMessage();
    }

    if (webSocketClient->connected()) {
      webSocketClient->stop();
    }

    signalKTaskHandle = NULL;
    vTaskDelete(NULL);
 }

void initSignalK() {

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing SignalK...\n");

  if (isSignalKActivated()) {
    biEnqueueSystemMessage(INFO, "Activating SignalK...\n");
 
    BaseType_t result = xTaskCreatePinnedToCore(
      signalKTask,        // Function to implement the task
      "SignalKTask",      // Name of the task
      8192,               // Stack size in words
      NULL,               // Task input parameter
      3,                  // Priority of the task
      &signalKTaskHandle, // Task handle
      0
    );
    if (result == pdPASS) {
      signalkEnabled = true;
      biEnqueueSystemMessage(INFO, "SignalK activated.\n");
    } else {
      signalkEnabled = false;
      biEnqueueSystemMessage(INFO, "SignalK activation failed.\n");
     }
  } else {
    biEnqueueSystemMessage(INFO, "SignalK shall not be ativated.\n");
  }

  biEnqueueSystemMessage(INFO, "Initializing SignalK finished.\n");
}

void deinitSignalK() {

  signalkEnabled = false;
  vTaskDelay(pdMS_TO_TICKS(2000));
  if (signalKTaskHandle != NULL) {
    vTaskDelete(signalKTaskHandle);
    signalKTaskHandle = NULL;
  }

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Deinitializing SignalK...\n");
}