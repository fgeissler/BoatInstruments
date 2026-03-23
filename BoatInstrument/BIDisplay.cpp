#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TouchDrvGT911.hpp>

#include <lvgl.h>
#include "lv_conf.h"

#include "BISystemMessage.h"


// Hardware-Konfiguration Rev 4
#define GT11_I2C_ADDR 0x5D
#define I2C_SDA 15
#define I2C_SCL 7

TouchDrvGT911 GT911;
int16_t x[5], y[5];
uint8_t gt911_i2c_addr = GT11_I2C_ADDR;


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


bool initGT911() {
  bool success = true;

  biEnqueueSystemMessage(OFF, "\n");
  biEnqueueSystemMessage(INFO, "Initializing GT911...\n");

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);
  
//  i2c_scan();
  
  if (gt911_i2c_addr == 0) {
    biEnqueueSystemMessage(WARN, "GT911 address not defined.");
    return false;
  }
  
  GT911.setPins(-1, -1);
  if (GT911.begin(Wire, gt911_i2c_addr, I2C_SDA, I2C_SCL)) {
    biEnqueueSystemMessage(INFO, "GT911 initialized successfully at address %d.", gt911_i2c_addr);
  
    GT911.setHomeButtonCallback([](void *user_data) {
    biEnqueueSystemMessage(INFO, "Home button pressed!");
  },
                              NULL);
  GT911.setMaxTouchPoint(1);  // max is 5
  } else {
    biEnqueueSystemMessage(WARN, "Failed to initialize GT911 at address %d.", gt911_i2c_addr);
    success = false;
  }

  biEnqueueSystemMessage(INFO, "Initializing GFX finished with %s\n", success ? "SUCCESS." : "ERROR.");
  return success;
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

  uint8_t touched = GT911.getPoint(x, y, GT911.getSupportTouchPoint());

  if (touched > 0) {
    unsigned long touchEvent = millis();
    
    for (int i = 0; i < touched; ++i) {
      int16_t touchX = x[i];
      int16_t touchY = y[i];
      switch (gfx->getRotation()) {
        case 0:
          break;
        case 1:
          touchX = y[i];
          touchY = gfx->height() - x[i];
          break;
        case 2:
          touchX = gfx->width() - x[i];
          touchY = gfx->height() - y[i];
          break;
        case 3:
          touchX = gfx->width() - y[i];
          touchY = x[i];
          break;
      }
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = LV_CLAMP(0, touchX, 479);
      data->point.y = LV_CLAMP(0, touchY, 479);

//      biEnqueueSystemMessage(INFO, "Touch %dms x: %d y: %d\n", touchEvent, touchX, touchY);
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
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
  lv_disp_set_bg_image(display, &ui_img_bg_main);
  lv_disp_set_bg_opa(display, LV_OPA_COVER); // Den Hintergrund vollflächig sichtbar machen
}

lv_obj_t * tileView;

void initTileView() {

  // Set background of Screen transparent, so that the image of the display is visible
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);

  // Create Tileview on main screen
  tileView = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(tileView, 480, 480); // width 480, height 480
  lv_obj_align(tileView, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Set background of Tileview transparent, so that the image of the display is visible
  lv_obj_set_style_bg_opa(tileView, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(tileView, 0, 0);

  // Remove Scrollbars
  lv_obj_set_scrollbar_mode(tileView, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(tileView, LV_DIR_HOR); // Nur horizontales Wischen erlau
}


static void anim_y_cb(void * object, int32_t value) {
  lv_obj_set_y((lv_obj_t *)object, value);
}

lv_anim_t animationStatusbar;

void statusBarShowAnimated(bool show) {
  if (isStatusBarVisible == show) {
    return;
  }

  isStatusBarVisible = show;

  lv_timer_set_period(autoHideTimer, 10000);

  if (show) {
    if (autoHideTimer) {
      lv_timer_reset(autoHideTimer); // Start time
      lv_timer_resume(autoHideTimer);
    } 
    lv_anim_set_values(&animationStatusbar, lv_obj_get_y(uiStatusBar), 7);
  } else {
    if (autoHideTimer) {
      lv_timer_pause(autoHideTimer); // Pause timer until it is activated by swipe down gesture
    } 
    lv_anim_set_values(&animationStatusbar, lv_obj_get_y(uiStatusBar), -35); // Height of status bar and shade
  }
  lv_anim_start(&animationStatusbar);
}

static void clock_timer_cb(lv_timer_t *timer) {
  struct tm timeInfo;

  if (getLocalTime(&timeInfo)) {
    lv_label_set_text_fmt(uiClockLabel, "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
  }
}

void initStatusBar() {

  lv_obj_t * topLayer = lv_layer_top();
  lv_obj_add_flag(topLayer, LV_OBJ_FLAG_EVENT_BUBBLE);

  // We use lv_layer_top() to keep uiStatusBar always on top of every other screen
  uiStatusBar = lv_obj_create(topLayer);

  // 2. Size and position, make it a litte smaler and 5px greater than necessary, so that we kan
  //    move it to pos -5 hiding the top rounded corners.
  lv_obj_set_size(uiStatusBar, 466, 35);
  lv_obj_align(uiStatusBar, LV_ALIGN_TOP_MID, 0, 7);

  // 3. Glas-Effekt: Weiß mit geringer Deckkraft
  lv_obj_set_style_bg_color(uiStatusBar, lv_color_hex(0x222222), 0);
  lv_obj_set_style_bg_opa(uiStatusBar, 120, 0);

  // 4. Rand und Kanten (Kein Rand, keine Rundung für den "nahtlosen" Abschluss oben)
  lv_obj_set_style_border_width(uiStatusBar, 0, 0);
  lv_obj_set_style_radius(uiStatusBar, 10, 0);

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


  // Time Label
  uiClockLabel = lv_label_create(uiStatusBar);
  lv_obj_align(uiClockLabel, LV_ALIGN_LEFT_MID, +10, 0);
  lv_obj_set_style_text_color(uiClockLabel, lv_color_white(), 0);

  // Start Timer for Clock updates
  lv_timer_create(clock_timer_cb, 1000, NULL);


  // Battery Icon
  uiBatteryIcon = lv_label_create(uiStatusBar);
  lv_label_set_text(uiBatteryIcon, LV_SYMBOL_BATTERY_EMPTY);
  lv_obj_align(uiBatteryIcon, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_set_style_text_color(uiBatteryIcon, lv_palette_main(LV_PALETTE_RED), 0);

  // WiFi Icon
  uiWiFiIcon = lv_label_create(uiStatusBar);
  lv_label_set_text(uiWiFiIcon, LV_SYMBOL_WIFI);
  lv_obj_align(uiWiFiIcon, LV_ALIGN_RIGHT_MID, -35, 0);
  lv_obj_set_style_text_color(uiWiFiIcon, lv_color_white(), 0);

  // Register for WiFi related messages
  // lv_msg_subscribe_obj(MSG_WIFI_CON, uiWiFiIcon, NULL);
  // lv_obj_add_event_cb(uiWiFiIcon, status_msg_cb, LV_EVENT_MSG_RECEIVED, NULL);

  lv_anim_init(&animationStatusbar);
  lv_anim_set_var(&animationStatusbar, uiStatusBar);
  lv_anim_set_time(&animationStatusbar, 550);
  lv_anim_set_exec_cb(&animationStatusbar, anim_y_cb);
  lv_anim_set_path_cb(&animationStatusbar, lv_anim_path_ease_out); // Smoth fade out

  // Create Auto Hide Timer
  autoHideTimer = lv_timer_create([](lv_timer_t *t) {
    statusBarShowAnimated(false);
  }, 30000, NULL);
  lv_timer_reset(autoHideTimer); // As LVGL will fire the timer immediatly we have to reset it first.

  // Special callback for top -> down gesture
  lv_indev_t * indev = lv_indev_get_next(NULL);
  indev->driver->feedback_cb = [](struct _lv_indev_drv_t * indev_drv, uint8_t event) {
    if(event == LV_EVENT_GESTURE) {
      // Geste abfragen
      lv_indev_t * indev = lv_indev_get_act();
      lv_dir_t dir = lv_indev_get_gesture_dir(indev);
      if (dir == LV_DIR_BOTTOM) {
        statusBarShowAnimated(true);
      }
    }
  };
}


lv_obj_t *biTileMain;
lv_obj_t *canvas1;
lv_obj_t *canvasTime;

#define G_WIDTH  120
#define G_HEIGHT 300
// Create dynamic buffer in PSRAM
static uint8_t *graph_buf_1;
static lv_coord_t last_x = G_WIDTH / 2; // Start in the middle

#define TIME_W 100
#define TIME_H 300
// Create dynamic buffer in PSRAM
uint8_t *time_buf;

void draw_background_grid(lv_obj_t * canvas) {
    lv_draw_line_dsc_t grid_dsc;
    lv_draw_line_dsc_init(&grid_dsc);
    grid_dsc.color = lv_color_white();
    grid_dsc.color = lv_color_black();
    grid_dsc.opa = 40; // Sehr dezent im Hintergrund
    grid_dsc.width = 1;
    grid_dsc.dash_width = 4; // Gestrichelte Linie
    grid_dsc.dash_gap = 4;

    lv_point_t p[2];
    
    // Vertikale Linien als Skala (bleiben fest stehen)
    for(int x_pos = 15; x_pos <= 85; x_pos += 35) {
        int x = (G_WIDTH * x_pos) / 100;
        p[0].x = x; p[0].y = G_HEIGHT - 3; // Nur im untersten (neuen) Bereich zeichnen
        p[1].x = x; p[1].y = G_HEIGHT - 1;
        lv_canvas_draw_line(canvas, p, 2, &grid_dsc);
    }
}

void update_vertical_graph(int next_x) {
  if (graph_buf_1 == NULL) {
    biEnqueueSystemMessage(WARN, "Buffer for graphics ist NULL.\n");
    return;
  }

  // 1. SCROLLEN: Schiebt Zeile 2 bis Ende auf Position 0
  int bytes_per_line = G_WIDTH * 3;
  int offset = 2 * bytes_per_line;
  int size_to_copy = (G_HEIGHT - 2) * bytes_per_line;
  
  memmove(graph_buf_1, graph_buf_1 + offset, size_to_copy);

  // 2. LÖSCHEN: Nur die untersten 3 Zeilen (Sicherheitsmarge) säubern
  // Wir löschen ab der drittletzten Zeile bis zum Ende
  int clear_start = (G_HEIGHT - 3) * bytes_per_line;
  int clear_size = 3 * bytes_per_line;
  memset(graph_buf_1 + clear_start, 0, clear_size); 

  // 3. NEU: Hilfslinien im untersten Bereich einzeichnen
  // Diese wandern nun mit dem Scrollen nach oben und bilden ein Gitter
  draw_background_grid(canvas1);

  // 4. ZEICHNEN: Neue Linie von Alt zu Neu
  lv_draw_line_dsc_t line_dsc;
  lv_draw_line_dsc_init(&line_dsc);
  line_dsc.color = lv_color_black();
  line_dsc.width = 2;

  // 5. ZEICHNEN: Ein Array mit zwei Punkten erstellen
  lv_point_t line_points[2];
  // Startpunkt (nach dem Scrollen auf G_HEIGHT - 3 gerutscht)
  line_points[0].x = (lv_coord_t)last_x;
  line_points[0].y = (lv_coord_t)(G_HEIGHT - 3);

  // Endpunkt (der neue Wert ganz unten)
  line_points[1].x = (lv_coord_t)next_x;
  line_points[1].y = (lv_coord_t)(G_HEIGHT - 1);

  // 6. Draw line  
  lv_canvas_draw_line(canvas1, line_points, 2, &line_dsc);
  
  last_x = next_x;
  
  // 4. WICHTIG: LVGL mitteilen, dass sich der Canvas-Puffer geändert hat
  lv_obj_invalidate(canvas1);
}

void update_time_waterfall(const char *time_str) {
 if (time_buf == NULL) {
    biEnqueueSystemMessage(WARN, "Buffer for Time Canvas ist NULL.\n");
    return;
  }

  // 1. Scrollen: Wir machen Platz für eine neue Zeile Text (ca. 30 Pixel hoch)
  int scroll_pixels = 2; 
  int byte_offset = scroll_pixels * TIME_W * 3;
  int bytes_to_copy = (TIME_H - scroll_pixels) * TIME_W * 3;
  
  memmove(time_buf, time_buf + byte_offset, bytes_to_copy);

  // 2. Den neuen Bereich unten leeren
  memset(time_buf + (TIME_H - scroll_pixels) * TIME_W * 3, 0, scroll_pixels * TIME_W * 3);

  if (NULL != time_str) {
    // 3. Den Text (Uhrzeit) unten in den Canvas schreiben
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_color_white();
    label_dsc.font = &lv_font_montserrat_14; // Wähle eine passende Größe
    label_dsc.align = LV_TEXT_ALIGN_CENTER;

    // Den Text am unteren Rand des Canvas platzieren
    lv_canvas_draw_text(canvasTime, 0, TIME_H - 25, TIME_W, &label_dsc, time_str);
  }  

  lv_obj_invalidate(canvasTime);
}

void graph_timer_cb(lv_timer_t * timer) {
  // Beispiel: Zufallswerte generieren (ersetze dies durch deine Sensordaten)
  // Wir nutzen LV_CLAMP, damit der Wert innerhalb der Canvas-Breite (G_WIDTH) bleibt
  int val1 = LV_CLAMP(5, rand() % G_WIDTH, G_WIDTH - 5);
//    int val2 = LV_CLAMP(5, rand() % G_WIDTH, G_WIDTH - 5);

  // Einfache Glättung (Exponential Moving Average)
  // Die Formel: 60% alter Wert + 40% neuer Wert
  // Je höher die 6 bei smooth_x, desto "träger" und weicher wird die Kurve
  static int smooth_x = G_WIDTH / 2;
  smooth_x = (smooth_x * 6 + val1 * 4) / 10;

  // Deine Scroll- und Zeichen-Logik für beide Graphen aufrufen
  update_vertical_graph(smooth_x);


  static int last_minute = -1;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    int stunde = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;
    int sekunde = timeinfo.tm_sec;

    if (minute != last_minute) {
      char buf[16];
      sprintf(buf, "- %02d:%02d -", stunde, minute);
      update_time_waterfall(buf);
      last_minute = minute;
    } else {
      update_time_waterfall(NULL);
    }
  }


//    update_vertical_graph_2(val2);
}

void initScreenMain() {

  // Tile #1: Column 0, Row 0)
  biTileMain = lv_tileview_add_tile(tileView, 0, 0, LV_DIR_HOR);

  // 2. Create main Panel inside this tile, all other widgets will be inside this panel
  lv_obj_t * mainPanel = lv_obj_create(biTileMain);

  // Important: Das Scroll-Event nach oben "durchreichen" (Bubbling)
  lv_obj_add_flag(mainPanel, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_clear_flag(mainPanel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(mainPanel, 466, 466);
  lv_obj_align(mainPanel, LV_ALIGN_TOP_MID, 0, 7);

  // 3. Hintergrund: Weiß mit starker Transparenz (ca. 20-40%)
  lv_obj_set_style_bg_color(mainPanel, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(mainPanel, 60, 0); // Wert von 0-255 (60 ist ca. 25%)

  // 4. Rand: Ein sehr feiner, fast weißer Rahmen (erzeugt die Glaskante)
  lv_obj_set_style_border_color(mainPanel, lv_color_white(), 0);
  lv_obj_set_style_border_opa(mainPanel, 100, 0); // Etwas sichtbarer als der Hintergrund
  lv_obj_set_style_border_width(mainPanel, 1, 0);

  // 5. Abgerundete Ecken für den modernen Look
  lv_obj_set_style_radius(mainPanel, 10, 0);

  // 6. Schatten entfernen oder sehr weich machen
  lv_obj_set_style_shadow_width(mainPanel, 20, 0);
  lv_obj_set_style_shadow_opa(mainPanel, 30, 0); // Ganz dezenter Schattenwurf

  // 7. Padding entfernen (optional, damit Inhalt nah am Rand sein kann)
  lv_obj_set_style_pad_all(mainPanel, 10, 0);

  lv_obj_t *uiLabel = lv_label_create(mainPanel);
  lv_obj_align(uiLabel, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_text_color(uiLabel, lv_color_white(), 0);
  lv_label_set_text(uiLabel, "Main Screen");



// Wind velocity
  // 480x480 Display, 16-Bit Color + 8-Bit Alpha = 3 Bytes
  uint32_t buffer_size = G_WIDTH * G_HEIGHT * 3; 
  graph_buf_1 = (uint8_t *)ps_malloc(buffer_size);
  memset(graph_buf_1, 0, buffer_size);

  canvas1 = lv_canvas_create(mainPanel); 
  lv_canvas_set_buffer(canvas1, (lv_color_t *)graph_buf_1, G_WIDTH, G_HEIGHT, LV_IMG_CF_TRUE_COLOR_ALPHA);
  // Den gesamten Canvas transparent machen (Alpha = 0)
  lv_canvas_fill_bg(canvas1, lv_color_white(), LV_OPA_TRANSP); 
  lv_obj_align(canvas1, LV_ALIGN_TOP_LEFT, 20, 50); // Position in panel
  lv_canvas_fill_bg(canvas1, lv_color_make(255, 255, 255), 0); // Transparent start (alpha 0)  

  // 3. Hintergrund: Weiß mit starker Transparenz (ca. 20-40%)
  lv_obj_set_style_bg_color(canvas1, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(canvas1, 100, 0); // Wert von 0-255 (60 ist ca. 25%)

  // 4. Rand: Ein sehr feiner, fast weißer Rahmen (erzeugt die Glaskante)
  lv_obj_set_style_border_color(canvas1, lv_color_white(), 0);
  lv_obj_set_style_border_opa(canvas1, 100, 0); // Etwas sichtbarer als der Hintergrund
  lv_obj_set_style_border_width(canvas1, 1, 0);

  // 5. Abgerundete Ecken für den modernen Look
  lv_obj_set_style_radius(canvas1, 10, 0);

  // 6. Schatten entfernen oder sehr weich machen
  lv_obj_set_style_shadow_width(canvas1, 20, 0);
  lv_obj_set_style_shadow_opa(canvas1, 30, 0); // Ganz dezenter Schattenwurf

  // 7. Padding entfernen (optional, damit Inhalt nah am Rand sein kann)
  lv_obj_set_style_pad_all(canvas1, 10, 0);
// Wind velocity

// Time
  // Puffer im PSRAM (3 Bytes pro Pixel für 16-Bit + Alpha)
  buffer_size = TIME_W * TIME_H * 3;
  time_buf = (uint8_t *)ps_malloc(buffer_size);
  memset(time_buf, 0, buffer_size);

  // Im Setup:
  canvasTime = lv_canvas_create(mainPanel);
  lv_canvas_set_buffer(canvasTime, (lv_color_t *)time_buf, TIME_W, TIME_H, LV_IMG_CF_TRUE_COLOR_ALPHA);
  lv_obj_align(canvasTime, LV_ALIGN_CENTER, 0, 0); // Genau in die Mitte
// Time


  // Timer erstellen: Alle 100ms (10 mal pro Sekunde)
  lv_timer_t *graph_timer = lv_timer_create(graph_timer_cb, 1000, NULL);

  // In deinem setup() beim Erstellen des Tileview:
  lv_obj_add_event_cb(tileView, [](lv_event_t * e) {
    lv_obj_t * tv_obj = lv_event_get_target(e);
    lv_obj_t * act_tile = lv_tileview_get_tile_act(tv_obj);

    // Jetzt ist 'graph_timer' hier drin bekannt!
    lv_timer_t * timer_ptr = (lv_timer_t *)lv_event_get_user_data(e);
    if(lv_obj_get_index(act_tile) == 0) {
        lv_timer_resume(timer_ptr);
    } else {
        lv_timer_pause(timer_ptr);
    }

 }, LV_EVENT_VALUE_CHANGED, graph_timer);
}


lv_obj_t *biTileConfiguration;

void initScreenConfiguration() {

 // Tile #2: Column 1, Row 0)
  biTileConfiguration = lv_tileview_add_tile(tileView, 1, 0, LV_DIR_HOR);

  // 2. Create main Panel inside this tile, all other widgets will be inside this panel
  lv_obj_t *mainPanel = lv_obj_create(biTileConfiguration);

 // Important: Das Scroll-Event nach oben "durchreichen" (Bubbling)
  lv_obj_add_flag(mainPanel, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_clear_flag(mainPanel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(mainPanel, 466, 466);
  lv_obj_align(mainPanel, LV_ALIGN_TOP_MID, 0, 7);

  // 3. Hintergrund: Weiß mit starker Transparenz (ca. 20-40%)
  lv_obj_set_style_bg_color(mainPanel, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(mainPanel, 60, 0); // Wert von 0-255 (60 ist ca. 25%)

  // 4. Rand: Ein sehr feiner, fast weißer Rahmen (erzeugt die Glaskante)
  lv_obj_set_style_border_color(mainPanel, lv_color_white(), 0);
  lv_obj_set_style_border_opa(mainPanel, 100, 0); // Etwas sichtbarer als der Hintergrund
  lv_obj_set_style_border_width(mainPanel, 1, 0);

  // 5. Abgerundete Ecken für den modernen Look
  lv_obj_set_style_radius(mainPanel, 10, 0);

  // 6. Schatten entfernen oder sehr weich machen
  lv_obj_set_style_shadow_width(mainPanel, 20, 0);
  lv_obj_set_style_shadow_opa(mainPanel, 30, 0); // Ganz dezenter Schattenwurf

  // 7. Padding entfernen (optional, damit Inhalt nah am Rand sein kann)
  lv_obj_set_style_pad_all(mainPanel, 10, 0);

  lv_obj_t *uiLabel = lv_label_create(mainPanel);
  lv_obj_align(uiLabel, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_text_color(uiLabel, lv_color_white(), 0);
  lv_label_set_text(uiLabel, "Configuration Screen");
}


void initScreens() {

  initTileView();
  initScreenMain();
  initScreenConfiguration();
}

void initDisplay() {
  initGT911();
  initGFX();
  initLv();
  initLVGLDisplay();
  initStatusBar();
  initScreens();
}

void biRemoveGFXSystemMessageLogger() {
  biDeregisterSystemMessageConsumer(gfxLogger);
}