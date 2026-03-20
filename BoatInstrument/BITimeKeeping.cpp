#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <HWCDC.h>
#include <Wire.h>
#include <time.h>
#include <esp_sntp.h>

#include "SensorPCF85063.hpp"

#include "BISystemMessage.h"

SensorPCF85063 rtc;


#define MY_NTP_SERVER "de.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

// Callback function (gets called when time adjusts via NTP) 
//triggered by sntp_set_time_sync_notification_cb( timeavailable );
void timeavailable(struct timeval *t) {
  tm tm;

  localtime_r(&t->tv_sec, &tm); // update the structure tm with the current time
  biEnqueueSystemMessage(INFO, "Got time adjustment from NTP! %d.%m.%Y %H:%M:%S.\n", &tm);
  rtc.setDateTime(tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void initClock() {
  time_t now; // this are the seconds since Epoch (1970) - UTC
  tm tm;      // the structure tm holds time information in a more convenient way *

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing Clock...\n");
 
  if (!rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, 15, 7)) {
    biEnqueueSystemMessage(INFO, "Failed to find PCF8563 - check your wiring!\n");
  } else {
    RTC_DateTime rtcDateTime = rtc.getDateTime();

    tm.tm_year = rtcDateTime.year-1900;
    tm.tm_mon = rtcDateTime.month;
    tm.tm_mday = rtcDateTime.day;
    tm.tm_hour = rtcDateTime.hour;
    tm.tm_min = rtcDateTime.minute;
    tm.tm_sec = rtcDateTime.second;

    time_t t = mktime(&tm);
    struct timeval current_time = { .tv_sec = t };
    settimeofday(&current_time, NULL);

    sntp_set_time_sync_notification_cb( timeavailable ); // set notification call-back function
  }

  sntp_set_sync_interval(12 * 60 * 60 * 1000UL); // 12 hours
  configTime(0, 0, MY_NTP_SERVER);  // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TZ, 1);           // Set environment variable with your time zone
  tzset();

  time(&now);             // read the current time
  localtime_r(&now, &tm); // update the structure tm with the current time
  biEnqueueSystemMessage(INFO, "Time is: %d.%m.%Y %H:%M:%S.\n", &tm);

  biEnqueueSystemMessage(INFO, "Initializing Clock finished.\n");
}
