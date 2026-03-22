#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include <lvgl.h>
#include "lv_conf.h"

#include "BISystemMessage.h"

LV_IMG_DECLARE(ui_img_bg_main);

#define BI_LVGL_TICK_PERIOD_MS 2

uint32_t screenWidth;
uint32_t screenHeight;

static lv_disp_draw_buf_t draw_buf;

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

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation */, true /* auto_flush */,
  bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));


void gfxLogger(const system_message_level_t level, const char *buffer) {
  gfx->print(buffer);
}

void lvglLogger(const system_message_level_t level, const char *buffer) {

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

  screenWidth = gfx->width();
  screenHeight = gfx->height();

  biRegisterSystemMessageConsumer(gfxLogger);

  biEnqueueSystemMessage(INFO, "Initializing GFX finished with %s\n", success ? "SUCCESS." : "ERROR.");
}

/* Display flushing */
void biLvglDisplayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void biTouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {

  // TODO

}

void lv_tick_task(void *arg) {
  lv_tick_inc(BI_LVGL_TICK_PERIOD_MS); // LVGL sagen, dass BI_LVGL_TICK_PERIOD_MS ms vergangen sind
}

void initLv() {

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing LV...\n");
 
  lv_init();

  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(screenWidth * screenHeight / 4 * sizeof(lv_color_t), MALLOC_CAP_DMA);

  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(screenWidth * screenHeight / 4 * sizeof(lv_color_t), MALLOC_CAP_DMA);

  String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() + "\n";
  biEnqueueSystemMessage(INFO, LVGL_Arduino.c_str());

  #if LV_USE_LOG != 0
    lv_log_register_print_cb(bi_lv_debug_log);
  #endif
  
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, screenWidth * screenHeight / 4);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = biLvglDisplayFlush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.sw_rotate = 1;  // add for rotation
//  disp_drv.rotated = LV_DISP_ROT_90;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = biTouchpadRead;
  lv_indev_drv_register(&indev_drv);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &lv_tick_task,
    .name = "lvgl_tick"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, BI_LVGL_TICK_PERIOD_MS * 1000);

  biRegisterSystemMessageConsumer(lvglLogger);
  biEnqueueSystemMessage(INFO, "LVGL is now registered as SystemMessageConsumer.\n");
  biEnqueueSystemMessage(INFO, "Initializing LV finished.\n");
}

lv_obj_t *uiStatusBar;
lv_obj_t *uiClockLabel;
lv_obj_t *uiWiFiIcon;
lv_obj_t *uiBatteryIcon;

static bool isStatusBarVisible = true;
static lv_timer_t *autoHideTimer = NULL;


lv_disp_t * display;

void initLVGLDisplay() {
  display = lv_disp_get_default();
  //lv_disp_set_bg_color(display, lv_palette_main(LV_PALETTE_BLUE)); // z.B. Grau
  lv_disp_set_bg_image(lv_disp_get_default(), &ui_img_bg_main);
  lv_disp_set_bg_opa(display, LV_OPA_COVER); // Den Hintergrund vollflächig sichtbar machen
}


static void anim_y_cb(void * statusBar, int32_t value) {
  lv_obj_set_y((lv_obj_t *)statusBar, value);
}

void statusBarShowAnimated(bool show) {
  if (isStatusBarVisible == show) {
    return;
  }

  isStatusBarVisible = show;

  lv_anim_t animation;
  lv_anim_init(&animation);
  lv_anim_set_var(&animation, uiStatusBar);
  lv_anim_set_time(&animation, 750);
  lv_anim_set_exec_cb(&animation, anim_y_cb);
  lv_anim_set_path_cb(&animation, lv_anim_path_ease_out); // Smoth fade out

  lv_timer_set_period(autoHideTimer, 10000);

  if (show) {
    if (autoHideTimer) {
      lv_timer_reset(autoHideTimer); // Start time
    } 
    lv_anim_set_values(&animation, lv_obj_get_y(uiStatusBar), 0);
  } else {
    if (autoHideTimer) {
      lv_timer_pause(autoHideTimer); // Pause timer until it is activated by swipe down gesture
    } 
    lv_anim_set_values(&animation, lv_obj_get_y(uiStatusBar), -40); // Height of status bar and shade
  }
  lv_anim_start(&animation);
}

static void clock_timer_cb(lv_timer_t *timer) {
  struct tm timeInfo;

  if (getLocalTime(&timeInfo)) {
    lv_label_set_text_fmt(uiClockLabel, "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
  }
}

void initStatusBar() {

  // We use lv_layer_top() to keep uiStatusBar always on top of every other screen
  uiStatusBar = lv_obj_create(lv_layer_top());

  // 2. Größe und Position (Ganz oben, volle Breite)
  lv_obj_set_size(uiStatusBar, 480, 40);
  lv_obj_align(uiStatusBar, LV_ALIGN_TOP_MID, 0, 0);

  // 3. Glas-Effekt: Weiß mit geringer Deckkraft
  lv_obj_set_style_bg_color(uiStatusBar, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(uiStatusBar, 90, 0); // Etwas weniger transparenter als das Dashboard

  // 4. Rand und Kanten (Kein Rand, keine Rundung für den "nahtlosen" Abschluss oben)
  lv_obj_set_style_border_width(uiStatusBar, 0, 0);
  lv_obj_set_style_radius(uiStatusBar, 0, 0);

  // 5. Trennlinie unten (optional, erzeugt eine feine Lichtkante zum Dashboard)
  lv_obj_set_style_border_side(uiStatusBar, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(uiStatusBar, 1, 0);
  lv_obj_set_style_border_color(uiStatusBar, lv_color_white(), 0);
  lv_obj_set_style_border_opa(uiStatusBar, 80, 0);

  // 6. Scrollen und Padding deaktivieren (damit Icons perfekt sitzen)
  lv_obj_clear_flag(uiStatusBar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(uiStatusBar, 0, 0);

  // 7. Slite shadow at the bottom
  lv_obj_set_style_shadow_width(uiStatusBar, 10, 0);
  lv_obj_set_style_shadow_opa(uiStatusBar, 40, 0);
  lv_obj_set_style_shadow_ofs_y(uiStatusBar, 2, 0);

  // Time
  uiClockLabel = lv_label_create(uiStatusBar);
  lv_obj_align(uiClockLabel, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_text_color(uiClockLabel, lv_color_white(), 0);

  // Start Timer for Clock updates
  lv_timer_create(clock_timer_cb, 1000, NULL);


  // Battery Icon
  uiBatteryIcon = lv_label_create(uiStatusBar);
  lv_label_set_text(uiBatteryIcon, LV_SYMBOL_BATTERY_EMPTY);
  lv_obj_align(uiBatteryIcon, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_text_color(uiBatteryIcon, lv_palette_main(LV_PALETTE_RED), 0);

  // WiFi Icon
  uiWiFiIcon = lv_label_create(uiStatusBar);
  lv_label_set_text(uiWiFiIcon, LV_SYMBOL_WIFI);
  lv_obj_align(uiWiFiIcon, LV_ALIGN_RIGHT_MID, -25, 0);
  lv_obj_set_style_text_color(uiWiFiIcon, lv_color_white(), 0);

  // Register for WiFi related messages
  // lv_msg_subscribe_obj(MSG_WIFI_CON, uiWiFiIcon, NULL);
  // lv_obj_add_event_cb(uiWiFiIcon, status_msg_cb, LV_EVENT_MSG_RECEIVED, NULL);


  // Create Auto Hide Timer
  autoHideTimer = lv_timer_create([](lv_timer_t *t) {
    statusBarShowAnimated(false);
  }, 30000, NULL);
  lv_timer_reset(autoHideTimer); // As LVGL will fire the time immediatly we have to reset it immediately.
}


lv_obj_t *biScreenMain;
lv_obj_t *biPainMain;

void initScreenMain() {

  // 1. Ein Objekt (z.B. ein Panel oder Button) erstellen
  biScreenMain = lv_scr_act();
  lv_obj_set_style_bg_opa(biScreenMain, 0, 0);
  lv_obj_t * biPainMain = lv_obj_create(biScreenMain);

  lv_obj_set_size(biPainMain, 466, 466);
  lv_obj_align(biPainMain, LV_ALIGN_TOP_MID, 0, 7);

  // 2. Hintergrund: Weiß mit starker Transparenz (ca. 20-40%)
  lv_obj_set_style_bg_color(biPainMain, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(biPainMain, 60, 0); // Wert von 0-255 (60 ist ca. 25%)

  // 3. Rand: Ein sehr feiner, fast weißer Rahmen (erzeugt die Glaskante)
  lv_obj_set_style_border_color(biPainMain, lv_color_white(), 0);
  lv_obj_set_style_border_opa(biPainMain, 100, 0); // Etwas sichtbarer als der Hintergrund
  lv_obj_set_style_border_width(biPainMain, 1, 0);

  // 4. Abgerundete Ecken für den modernen Look
  lv_obj_set_style_radius(biPainMain, 10, 0);

  // 5. Schatten entfernen oder sehr weich machen
  lv_obj_set_style_shadow_width(biPainMain, 20, 0);
  lv_obj_set_style_shadow_opa(biPainMain, 30, 0); // Ganz dezenter Schattenwurf

  // 6. Padding entfernen (optional, damit Inhalt nah am Rand sein kann)
  lv_obj_set_style_pad_all(biPainMain, 10, 0);
}


lv_obj_t *biScreenConfiguration;

void initScreens() {

  initScreenMain();

  biScreenConfiguration = lv_obj_create(NULL);

  auto gesture_cb = [](lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_obj_t *active = lv_scr_act();

    if (dir == LV_DIR_LEFT && active == biScreenMain) {
      lv_scr_load_anim(biScreenConfiguration, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    } else if (dir == LV_DIR_RIGHT && active == biScreenConfiguration) {
      lv_scr_load_anim(biScreenMain, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
  };

  // Allow gesture on all Screens
  lv_obj_add_event_cb(biScreenMain, gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(biScreenConfiguration, gesture_cb, LV_EVENT_GESTURE, NULL);
}

void initDisplay() {
  initGFX();
  initLv();
  initLVGLDisplay();
  initStatusBar();
  initScreens();
}

void biRemoveGFXSystemMessageLogger() {
  biDeregisterSystemMessageConsumer(gfxLogger);
}