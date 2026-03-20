#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <HWCDC.h>

#include <WebSocketsClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#
#include "BIDataStore.h"

//#include "signalk_config.h"

extern Arduino_RGB_Display *gfx;
extern HWCDC USBSerial;

static bool signalkEnabled = false;
static WebSocketsClient ws_client;
static TaskHandle_t signalk_task_handle = NULL;


// Outgoing message queue (simple ring buffer)
static SemaphoreHandle_t ws_queue_mutex = NULL;
static const int OUTGOING_QUEUE_SIZE = 8;
static String outgoing_queue[OUTGOING_QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;

static bool enqueue_outgoing(const String &msg) {
    if (ws_queue_mutex == NULL) return false;
    if (xSemaphoreTake(ws_queue_mutex, pdMS_TO_TICKS(100))) {
        if (queue_count >= OUTGOING_QUEUE_SIZE) {
            // Drop oldest to make room
            queue_head = (queue_head + 1) % OUTGOING_QUEUE_SIZE;
            queue_count--;
        }
        outgoing_queue[queue_tail] = msg;
        queue_tail = (queue_tail + 1) % OUTGOING_QUEUE_SIZE;
        queue_count++;
        xSemaphoreGive(ws_queue_mutex);
        return true;
    }
    return false;
}

static void flush_outgoing() {
    if (ws_queue_mutex == NULL) return;
    if (!ws_client.isConnected()) return;
    if (!xSemaphoreTake(ws_queue_mutex, pdMS_TO_TICKS(100))) return;
    while (queue_count > 0 && ws_client.isConnected()) {
        String &m = outgoing_queue[queue_head];
        ws_client.sendTXT(m);
        queue_head = (queue_head + 1) % OUTGOING_QUEUE_SIZE;
        queue_count--;
    }
    xSemaphoreGive(ws_queue_mutex);
}

// Public enqueue wrapper (declared in header)
void enqueue_signalk_message(const String &msg) {
    if (ws_queue_mutex == NULL) return;
    enqueue_outgoing(msg);
}

// FreeRTOS task for Signal K updates (runs on core 0)
// Task to run the WebSocket loop
static void signalk_task(void *parameter) {
    Serial.println("Signal K WebSocket task started");
    vTaskDelay(pdMS_TO_TICKS(500));

    while (signalk_enabled) {

        ws_client.loop();
        // Drain any messages queued from other tasks (e.g. refresh_signalk_subscriptions
        // called from the HTTP handler on Core 1). Only safe to call sendTXT() from here.
        flush_outgoing();

        unsigned long now = millis();

        // send periodic ping if connected
        if (ws_client.isConnected()) {
            if (now - last_message_time >= PING_INTERVAL_MS) {
                ws_client.sendPing();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    USBSerial.println("SignalK WebSocket task ended");
    vTaskDelete(NULL);
}

// Enable Signal K with WiFi credentials
void enable_signalk(const String& server_ip, int server_port) {
  if (signalkEnabled) {
    USBSerial.println("Signal K already enabled");
    return;
  }

  signalkEnabled = true;

  // Get all paths from configuration including gauges, number displays, and dual displays
  std::vector<String> all_paths = get_all_signalk_paths();

  // First, load the traditional gauge paths into signalk_paths array
//  for (int i = 0; i < TOTAL_PARAMS; i++) {
//      signalk_paths[i] = get_signalk_path_by_index(i);
//  }
    
    // Initialize mutex first
    init_sensor_mutex();
    // create ws queue mutex
    if (ws_queue_mutex == NULL) {
        ws_queue_mutex = xSemaphoreCreateMutex();
    }
    
    // WiFi should already be connected from setup_sensESP()
    if (WiFi.status() != WL_CONNECTED) {
        USBSerial.println("SignalK: WiFi not connected, aborting");
        signalkEnabled = false;
        return;
    }
    USBSerial.println("SignalK: Starting WebSocket client...");

    // Initialize websocket client
    ws_client.begin(getSignalKIP().c_str(), getSignalKPort(), "/signalk/v1/stream");
    ws_client.onEvent(wsEvent);
    // We'll manage reconnection with backoff ourselves
    ws_client.setReconnectInterval(0);

    // Create task to pump ws loop.
    // NOTE: xTaskCreateStaticPinnedToCore with a PSRAM stack fails with
    // "xPortcheckValidStackMem" assert unless CONFIG_FREERTOS_TASK_STACK_IN_PSRAM
    // is set in sdkconfig at IDF build time — a pre-compiled library check that
    // a -D flag cannot override.  Use xTaskCreatePinnedToCore (internal RAM stack).
    xTaskCreatePinnedToCore(signalk_task, "SignalKWS", 8192, NULL, 3, &signalk_task_handle, 0);

    USBSerial.println("SignalK WebSocket task created successfully.");
    USBSerial.flush();
}


void initSignalK() {

  gfx->println("\nInitializing SignalK...");
  USBSerial.println("\nInitializing SignalK...");

  if (isSignalKActivated()) {
    gfx->println("Activating SignalK...");
    USBSerial.println("Activating SignalK...");
//    enable_signalk(getSignalKIP(), getSignalKPort());
  }

  gfx->println("Initializing SignalK finished.");
  USBSerial.println("Initializing SignalK finished.");
}

void deinitSignalK() {

  gfx->println("\nDeinitializing SignalK...");
  USBSerial.println("\Deinitializing SignalK...");
}

