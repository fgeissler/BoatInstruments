#ifndef BI_DATA_STORE_H
#define BI_DATA_STORE_H

void initDataStore();

// WiFi
bool isWiFiActivated();
void activateWiFi();
void deactivateWiFi();

bool isWiFiAPMode();
void setWiFiAPMode(const String& apModeFromString);
void setWiFiAPMode();
void setWiFiClientMode();

const String& getWiFiHostName();
void setWiFiHostName(const String& hostName);

const String& getWiFiSSID();
void setWiFiSSID(const String& ssid);

const String& getWiFiPassword();
void setWiFiPassword(const String& password);
// WiFi

// Serial
bool getUseSerial();
void setUseSerial(bool doUseSerial);
// Serial

// SignalK
bool isSignalKActivated();
void setSignalKActivated(const String& activateSignalK);
const String& getSignalKIP();
void setSignalKIP(const String& newSignalKIP);
int getSignalKPort();
void setSignalKPort(int newSignalKPort);
// SignalK


#endif // BI_DATA_STORE_H