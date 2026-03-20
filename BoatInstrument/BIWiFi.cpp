#include <Arduino.h>
#include <WiFi.h>

#include "BISystemMessage.h"
#include "BIDataStore.h"


void initWiFi() {
  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing WiFI...\n");

  if (isWiFiActivated()) {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(getWiFiHostName().c_str()); //define hostname

    if (isWiFiAPMode()) {
      biEnqueueSystemMessage(INFO, "Activating WiFi as AP '%s'.\n", getWiFiSSID());
  
      WiFi.mode(WIFI_AP);
      WiFi.softAP(getWiFiSSID(), getWiFiPassword());

      biEnqueueSystemMessage(INFO, "IPAddress %s.\n", WiFi.softAPIP().toString().c_str());
   } else {
      biEnqueueSystemMessage(INFO, "Connecting via WiFi to AP '%s'.\n", getWiFiSSID());
 
      WiFi.mode(WIFI_STA);
      WiFi.begin(getWiFiSSID(), getWiFiPassword());

      while (WiFi.status() != WL_CONNECTED) {
        biEnqueueSystemMessage(OFF, ".");
        delay(1000);
      }
      biEnqueueSystemMessage(OFF, "\n");
      biEnqueueSystemMessage(INFO, "IPAddress %s.\n", WiFi.localIP().toString().c_str());
    }

    biEnqueueSystemMessage(INFO, "Initializing WiFi finished.\n");
  }
}
