#include <Arduino.h>
#include "freertos/queue.h"

#include <iostream>
#include <vector>

#include "BISystemMessage.h"


typedef struct {
    system_message_level_t level;
    char message[128]; // Puffer für die Nachricht
} system_message_event_t;

static QueueHandle_t systemMessageQueue = NULL;

// Die "Liste" als C++ Vector
static std::vector<bi_system_message_consumer_t> systemMessageConsumers;

bool biRegisterSystemMessageConsumer(bi_system_message_consumer_t systemMessageConsumer) {
  if (systemMessageConsumer == NULL) {
    return false;
  }

  systemMessageConsumers.push_back(systemMessageConsumer);

  return 0; // Erfolg
}

void biEnqueueSystemMessage(const system_message_level_t level, const char* format, ...) {
  va_list args;
  va_start(args, format); // Initialisierung mit dem letzten festen Parameter

  system_message_event_t systemMessageEvent;
  systemMessageEvent.level = level;
  vsnprintf(systemMessageEvent.message, sizeof(systemMessageEvent.message), format, args);

  // Nachricht in die Queue schicken (nicht blockierend)
  xQueueSend(systemMessageQueue, &systemMessageEvent, 0); 
}


char* asStr(system_message_level_t level) {
  switch (level) {
    case OFF: return   "";
    case TRACE: return "[TRACE] ";
    case DEBUG: return "[DEBUG] ";
    case INFO: return  "[INFO ] ";
    case WARN: return  "[WARN ] ";
    case ERROR: return "[ERROR] ";
    case FATAL: return "[FATAL] ";
    default: return    "[HÄ   ] ";
  }
}

// Nachrichten an alle verteilen
void biDispatchMessage(const system_message_level_t level, const char *message) {
  // Moderne Range-based for loop
  for (auto systemMessageConsumer : systemMessageConsumers) {
      systemMessageConsumer(level, message);
  }
}

void systemMessagePublisherTask(void *pvParameters) {
  system_message_event_t systemMessageEvent;

  while (1) {
    // Task schläft hier, bis ein Element in der Queue landet
    if (xQueueReceive(systemMessageQueue, &systemMessageEvent, portMAX_DELAY)) {
      // Hier rufen wir die Consumer-Liste auf
      biDispatchMessage(systemMessageEvent.level, systemMessageEvent.message);
    }
  }
}

void biInitSystemMessage() {
  systemMessageQueue = xQueueCreate(10, sizeof(system_message_event_t));

  if (systemMessageQueue != NULL) {
      // Task erstellen (niedrige Priorität reicht oft für Logging)
      xTaskCreate(systemMessagePublisherTask, "systemMessagePublisherTask", 3072, NULL, 1, NULL);
  }
}
