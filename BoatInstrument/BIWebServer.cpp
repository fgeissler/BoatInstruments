#include <Arduino.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "BISystemMessage.h"
#include "BIDataStore.h"
#include "BISignalK.h"

AsyncWebServer configServer(80); // Create an async web server object


// Minimal HTML style used by the configuration web pages.
// Stored as a plain const char[] so the data stays in flash (.rodata)
// instead of being heap-copied like a const String would be (~2.8 KB saved).
static const char STYLE[] PROGMEM = R"rawliteral(
<style>
body{font-family:Arial,Helvetica,sans-serif;background:#fff;color:#111}
.container{max-width:900px;margin:0 auto;padding:12px}
.tab-btn{background:#f4f6fa;border:1px solid #d8e0ef;border-radius:4px;padding:8px 12px;font-size: 14px;cursor:pointer}
.tab-tense-btn{background-color:Lavender;border:1px solid LightSteelBlue;border-radius:4px;padding:8px 12px;font-size: 14px;cursor:pointer}
.tab-crit-btn{background-color:Tomato;border:1px solid OrangeRed;border-radius:4px;padding:8px 12px;font-size: 14px;cursor:pointer}
.tab-content{border:1px solid #e6e9f2;padding:12px;border-radius:6px;background:#fff}
input[type=number]{width:90px}

/* Icon section styling */
.icon-section{display:flex;flex-direction:column;background:linear-gradient(180deg, #f7fbff, #ffffff);border:1px solid #dbe8ff;padding:10px;border-radius:6px;margin-bottom:8px;box-shadow:0 1px 0 rgba(0,0,0,0.02)}
.icon-section > .icon-row{display:flex;gap:12px;align-items:center}
.icon-section label{font-weight:600}
.icon-preview{width:48px;height:48px;border-radius:6px;background:#fff;border:1px solid #e6eefc;display:inline-block;overflow:hidden;display:flex;align-items:center;justify-content:center}
.icon-section .zone-row{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-top:6px}
.icon-section .zone-item{min-width:150px}
.icon-section .zone-item.small{min-width:90px}
.icon-section .color-input{width:40px;height:28px;padding:0;border:0;background:transparent}
.tab-content h3{margin-top:0;color:#1f4f8b}
/* Root page helpers */
.status{background:#f1f7ff;border:1px solid #dbe8ff;padding:10px;border-radius:6px;margin-bottom:12px;color:#0b2f5a}
.root-actions{display:flex;justify-content:center;gap:12px;margin-top:8px}
/* Screens selector container */
.screens-container{background:linear-gradient(180deg,#f0f7ff,#ffffff);border:1px solid #cfe6ff;padding:10px;border-radius:8px;margin-bottom:12px;display:flex;flex-direction:column;align-items:center}
.screens-container .screens-row{display:flex;gap:8px;flex-wrap:wrap;justify-content:center}
.screens-container .screens-title{width:100%;text-align:center;margin-bottom:6px;font-weight:700;color:#0b3b6a}
/* Form helpers */
.form-row{display:flex;flex-direction:row;align-items:center;gap:8px;margin-bottom:10px}
.form-row label{width:140px;text-align:right;color:#0b3b6a}
input[type=text],input[type=password]{width:60%;padding:6px;border:1px solid #dfe9fb;border-radius:4px}
input[type=number]{width:120px;padding:6px;border:1px solid #dfe9fb;border-radius:4px}

/* Assets manager styles */
.assets-uploader{display:flex;gap:8px;align-items:center;justify-content:center;margin-bottom:12px}
.assets-uploader input[type=file]{border:1px dashed #cfe3ff;padding:6px;border-radius:4px;background:#fbfdff}
.file-table{width:100%;border-collapse:collapse;margin-top:8px}
.file-table th{background:#f4f8ff;border-bottom:1px solid #dbe8ff;padding:8px;text-align:left;color:#0b3b6a}
.file-table td{padding:8px;border-bottom:1px solid #eef6ff}
.file-actions form{display:inline;margin-right:8px}
.file-size{color:#5877a8}

/* Calibration table styles */
.table{width:auto;border-collapse:collapse;margin-bottom:8px}
.table th{background:#f4f8ff;border-bottom:1px solid #dbe8ff;padding:4px 6px;text-align:left;color:#0b3b6a;font-weight:600;font-size:0.9em}
.table td{padding:4px 6px;border-bottom:1px solid #eef6ff;white-space:nowrap}
.table td:first-child{width:35px;text-align:center}
.table td:last-child{width:65px;text-align:center}
.table input[type=number]{width:65px;padding:3px 4px;font-size:0.9em}

</style>
)rawliteral";

void handleRootPage(AsyncWebServerRequest *request) {
  String html = "<html><head>";
  html += STYLE;
  html += "<title>BoatInstrument - Setup</title></head><body><div class='container'>";
  html += "<div class='tab-content'>";
  html += "<h2>Main Setup</h2>";
  html += "<div class='form-row'><a href='/network'>Network settings</a></div>";
  html += "<div class='form-row'><a href='/signalk'>SignalK settings</a></div>";
  html += "</div></div></body></html>";

  request->send(200, "text/html", html);
}

void handleNetworkPage(AsyncWebServerRequest *request) {
  String html = "<html><head>";
  html += STYLE;
  html += "<title>BoatInstrument - Network Setup</title></head><body><div class='container'>";
  html += "<div class='tab-content'>";
  html += "<h2>Network Setup</h2>";
  html += "<form method='POST' action='/save-wifi'>";
  html += "<div class='form-row'><label>APMode:</label><input name='apMode' type='checkbox' value='checked'";
  if (isWiFiAPMode()) {
     html += " checked";
  }
  html += "></div>";
  html += "<div class='form-row'><label>SSID:</label><input name='ssid' type='text' value='" + getWiFiSSID() + "'></div>";
  html += "<div class='form-row'><label>Password:</label><input name='password' type='password' value='" + getWiFiPassword() + "'></div>";
  html += "<div class='form-row'><label>ESP32 Hostname:</label><input name='hostname' type='text' value='" + getWiFiHostName() + "'></div>";
  html += "<div style='text-align:right;margin-top:12px;'>";
  html += "<button class='tab-btn' type='button' onclick='window.location.replace(\"/\")' style='padding:10px 18px;'>Back</button>";
  html += "<button class='tab-crit-btn' type='submit' style='margin-left:20px;padding:10px 18px;'>Save & Reboot</button></div>";
  html += "</form>";
  html += "</div></div></body></html>";

  request->send(200, "text/html", html);
}

// Task function to handle reboot
void rebootTask(void *pvParameters) {

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Starting Reboot Thread...\n");

  vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait 3 seconds

  biEnqueueSystemMessage(INFO, "Delay is over, resetting device...\n");
  ESP.restart(); // Clean restart
  vTaskDelete(NULL); // Should not reach here
}

void handleNetworkSave(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        setWiFiAPMode(request->arg("apMode"));
        setWiFiSSID(request->arg("ssid"));
        setWiFiPassword(request->arg("password"));
        setWiFiHostName(request->arg("hostname"));

        String html = "<html><head>";
        html += STYLE;
        html += "<meta http-equiv='refresh' content='10;url=/' />";
        html += "<style>#loading_indicator { visibility: visible; position: absolute; top: 0; bottom: 0; left: 0; right: 0; margin: auto; border: 10px solid grey; border-radius: 50%; border-top: 10px solid blue; width: 100px; height: 100px; animation: spinIndicator 1s linear infinite; }";
        html += "@keyframes spinIndicator { 100% { transform: rotate(360deg); } }</style>";
        html += "<title>Saved</title></head><body><div class='container'>";
        html += "<h2>Settings saved.<br>Rebooting...</h2>";
        html += "</div></body></html>";

        xTaskCreate(
          rebootTask,    // Function to implement the task
          "RebootTask",  // Name of the task
          2048,          // Stack size in words
          NULL,          // Task input parameter
          1,             // Priority of the task
          NULL           // Task handle
        );

        request->send(200, "text/html", html);
    } else {
        request->send(405, "text/plain", "Method Not Allowed");
    }
}

void handleSignalKPage(AsyncWebServerRequest *request) {
  String html = "<html><head>";
  html += STYLE;
  html += "<title>BoatInstrument - SignalK Setup</title></head><body><div class='container'>";
  html += "<div class='tab-content'>";
  html += "<h2>SignalK Setup</h2>";
  html += "<form method='POST' action='/save-signalk'>";
  html += "<div class='form-row'><label>SignalK:</label><input name='signalK' type='checkbox' value='checked'";
  if (isSignalKActivated()) {
     html += " checked";
  }
  html += "></div>";;
  html += "<div class='form-row'><label>SignalK Server:</label><input name='signalk_ip' type='text' value='" + getSignalKIP() + "'></div>";
  html += "<div class='form-row'><label>SignalK Port:</label><input name='signalk_port' type='number' value='" + String(getSignalKPort()) + "'></div>";
  html += "<div style='text-align:right;margin-top:12px;'>";
  html += "<button class='tab-btn' type='button' onclick='window.location.replace(\"/\")' style='padding:10px 18px;'>Back</button>";
  html += "<button class='tab-tense-btn' type='submit' style='margin-left:20px;padding:10px 18px;'>Save</button></div>";
  html += "</form>";
  html += "</div></div></body></html>";

  request->send(200, "text/html", html);
}

// Task function to handle reboot
void activateSignalK(void *pvParameters) {

  initSignalK();
  
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 3 seconds
  vTaskDelete(NULL);
}

void handleSignalKSave(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        setSignalKActivated(request->arg("signalK"));
        setSignalKIP(request->arg("signalk_ip"));
        setSignalKPort(request->arg("signalk_port").toInt());

        xTaskCreatePinnedToCore(
          activateSignalK, // Function to implement the task
          "RebootTask",    // Name of the task
          2048,            // Stack size in words
          NULL,            // Task input parameter
          1,               // Priority of the task
          NULL,            // Task handle
          1                // CPU 1
        );

        handleRootPage(request);
    } else {
        request->send(405, "text/plain", "Method Not Allowed");
    }
}

const char* http_username = "admin";
const char* http_password = "admin";


void initHTTPServer() {

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing HTTPServer...\n");

  // Set up the web server to handle different routes
  // Route for root / web page
  configServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    handleRootPage(request);
  });
 
  configServer.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(401);
  });

/*  configServer.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", logout_html, processor);
  });
*/
  configServer.on("/network", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    handleNetworkPage(request);
  });

  configServer.on("/save-wifi", HTTP_POST, [](AsyncWebServerRequest *request) { 
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    handleNetworkSave(request);
  });

  configServer.on("/signalk", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    handleSignalKPage(request);
  });

  configServer.on("/save-signalk", HTTP_POST, [](AsyncWebServerRequest *request) { 
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    handleSignalKSave(request);
  });

  // catch any request, and send a 404 Not Found response
  configServer.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });


  biEnqueueSystemMessage(INFO, "Initializing HTTPServer finished.\n");

  biEnqueueSystemMessage(INFO, "Starting HTTPServer on port 80.\n");

  // Start the web server
  configServer.begin();

  biEnqueueSystemMessage(INFO, "HTTPServer started.\n");
}

