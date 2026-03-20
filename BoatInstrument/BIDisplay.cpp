#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include <lvgl.h>
#include "lv_conf.h"

#include "BISystemMessage.h"


#if LV_USE_LOG != 0
/* Serial debugging */
void bi_lv_debug_log(const char *buf) {
  biEnqueueSystemMessage(INFO, buf);
}
#endif


Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 42 /* CS */,
    2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
    5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

// TOD Set Rotation to 0
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));


void gfxLogger(const system_message_level_t level, const char *buffer) {
  gfx->print(buffer);
}


void initGFX() {

  bool success = true;

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing GFX...\n");

  if (!gfx->begin()) {
    biEnqueueSystemMessage(INFO, "gfx->begin() failed!\n");
    success = false;
  }

  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_LIGHTGREY);
  gfx->setTextSize(2);

  biRegisterSystemMessageConsumer(gfxLogger);

  biEnqueueSystemMessage(INFO, "Initializing GFX finished with %s\n", success ? "SUCCESS." : "ERROR.");
}

void lv_tick_task(void *arg) {
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms warten
        lv_tick_inc(10);                // LVGL sagen, dass 10ms vergangen sind
    }
}

void initLv() {

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing LV...\n");
 
  lv_init();

  // Task fuer LV Tick erstellen und starten
  xTaskCreatePinnedToCore(lv_tick_task, "lv_tick", 1024, NULL, 1, NULL, 0);

  #if LV_USE_LOG != 0
    lv_log_register_print_cb(bi_lv_debug_log);
  #endif
  
  // TODO
  // Handle 180° rotation in LVGL (works with partial rendering)
  //lv_display_t *disp = lv_display_create(480, 480);
  //lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
  //lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);

  biEnqueueSystemMessage(INFO, "Initializing LV finished.\n");
}


void initDisplay() {
  initGFX();
  initLv();
}