#include <Arduino.h>
#include <Preferences.h>
#include <HWCDC.h>

#include "BISystemMessage.h"

Preferences preferences;


// WiFi
bool wifiActivated = false;
bool wifiAPMode = false;
String wifiHostName;
String wifiSSID;
String wifiPassword;
// WiFi

// Serial
bool useSerial = true;
// Serial

//SignalK
bool signalkActivated;
String signalKIP;
int signalKPort;
//SignalK

void initDataStore() {

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing DataStore...\n");

  // WiFi
  preferences.begin("wifi", true);
  wifiActivated = preferences.getBool("isActivated", true);
  wifiAPMode = preferences.getBool("modeAP", true);
  wifiHostName = preferences.getString("hostname", "Amal-BoatInstrument");
  wifiSSID = preferences.getString("ssid", "AmalBoatInstrument");
  wifiPassword = preferences.getString("password", "AmalBoatInstrument");
  preferences.end();
//  wifiSSID = "Heldin";
//  wifiPassword = "Quester+Kirsten+Februar+2025";
  // WiFi

  // Serial
  preferences.begin("serial", true);
  useSerial = preferences.getBool("useSerial", true);
  preferences.end();
  // Serial

  // SignalK
  preferences.begin("signalK", true);
  signalkActivated = preferences.getBool("isActivated", false);
  signalKIP = preferences.getString("signalKIP", "192.168.1.100");
  signalKPort = preferences.getInt("signalKPort", 9000);
  preferences.end();
  // SignalK

  biEnqueueSystemMessage(INFO, "Initializing DataStore finished.\n");
}

bool isWiFiActivated() {
  return wifiActivated;
}

void activateWiFi() {
  if (!wifiActivated) {
    biEnqueueSystemMessage(INFO, "DS: Activting WiFi.\n");
    wifiActivated = true;
    preferences.begin("wifi", false);
    preferences.putBool("isActivated", wifiActivated);
    preferences.end();
  }
}

void deactivateWiFi() {
  if (wifiActivated) {
    biEnqueueSystemMessage(INFO, "DS: Deactivting WiFi.\n");
    wifiActivated = false;
    preferences.begin("wifi", false);
    preferences.putBool("isActivated", wifiActivated);
    preferences.end();
  }
}

bool isWiFiAPMode() {
  return wifiAPMode;
}

void setWiFiAPMode() {
  if (!wifiAPMode) {
    biEnqueueSystemMessage(INFO, "DS: Activting APMode.\n");
    wifiAPMode = true;
    preferences.begin("wifi", false);
    preferences.putBool("modeAP", wifiAPMode);
    preferences.end();
  }
}

void setWiFiClientMode() {
  if (wifiAPMode) {
    biEnqueueSystemMessage(INFO, "DS: Activting Client mode.\n");
    wifiAPMode = false;
    preferences.begin("wifi", false);
    preferences.putBool("modeAP", wifiAPMode);
    preferences.end();
  }
}

void setWiFiAPMode(const String& apModeFromString) {
  biEnqueueSystemMessage(INFO, "DS: Setting APMode from string: %s.\n", apModeFromString);
  if (!apModeFromString.isEmpty() && apModeFromString.equalsIgnoreCase("checked")) {
    setWiFiAPMode();
  } else {
    setWiFiClientMode();
  }
}

const String& getWiFiHostName() {
  return wifiHostName;
}

void setWiFiHostName(const String& hostName) {
  if (!hostName.isEmpty() && !hostName.equals(wifiHostName)) {
    biEnqueueSystemMessage(INFO, "DS: Setting hostname to %s.\n", wifiHostName);
    wifiHostName = hostName;
    preferences.begin("wifi", false);
    preferences.putString("hostname", wifiHostName);
    preferences.end();
  }
}

const String& getWiFiSSID() {
  return wifiSSID;
}

void setWiFiSSID(const String& ssid) {
  if (!ssid.isEmpty() && !ssid.equals(wifiSSID)) {
    wifiSSID = ssid;
    preferences.begin("wifi", false);
    preferences.putString("ssid", wifiSSID);
    preferences.end();
  }
}

const String& getWiFiPassword() {
  return wifiPassword;
}

void setWiFiPassword(const String& password) {
  if (!password.isEmpty() && !password.equals(wifiPassword)) {
    wifiPassword = password;
    preferences.begin("wifi", false);
    preferences.putString("password", wifiPassword);
    preferences.end();
  }
}

bool getUseSerial() {
  return useSerial;
}

void setUseSerial(bool doUseSerial) {
  if (useSerial != doUseSerial) {
    useSerial = doUseSerial;
    preferences.begin("serial", false);
    preferences.putBool("useSerial", useSerial);
    preferences.end();
  }
}

// SignalK
bool isSignalKActivated() {
  return signalkActivated;
}

void setSignalKActivated(const String& activateSignalK) {
  bool newSignalKActivated = false;
  if ( !activateSignalK.isEmpty() && (activateSignalK.equalsIgnoreCase("true") || activateSignalK.equalsIgnoreCase("checked"))) {
    newSignalKActivated = true;
  }

  if (newSignalKActivated != signalkActivated) {
    signalkActivated = newSignalKActivated;
    preferences.begin("signalK", false);
    preferences.putBool("isActivated", signalkActivated);
    preferences.end();
  }
}

const String& getSignalKIP() {
  return signalKIP;
}

void setSignalKIP(const String& newSignalKIP) {
  if (!newSignalKIP.isEmpty() && !newSignalKIP.equals(signalKIP)) {
    signalKIP = newSignalKIP;
    preferences.begin("signalK", false);
    preferences.putString("signalKIP", signalKIP);
    preferences.end();
  }
}

int getSignalKPort() {
  return signalKPort;
}
void setSignalKPort(int newSignalKPort) {
  if (signalKPort != newSignalKPort) {
    signalKPort = newSignalKPort;
    preferences.begin("signalK", false);
    preferences.putInt("signalKPort", signalKPort);
    preferences.end();
  }
}

// SignalK

