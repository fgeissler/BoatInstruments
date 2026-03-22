#ifndef BI_SYSTEM_MASSAGE_H
#define BI_SYSTEM_MASSAGE_H

typedef enum {
  OFF,
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  FATAL
} system_message_level_t;

/**
 * System Message Consumer receive function. Receives a string buffer process by the consumer".
 */
typedef void (*bi_system_message_consumer_t)(const system_message_level_t level, const char *buffer);

void initSystemMessage();
bool biRegisterSystemMessageConsumer(bi_system_message_consumer_t systemMessageConsumer);
bool biDeregisterSystemMessageConsumer(bi_system_message_consumer_t systemMessageConsumer);
void biEnqueueSystemMessage(const system_message_level_t level, const char* format, ...);

char* asStr(system_message_level_t level);

#endif // BI_SYSTEM_MASSAGE_H