
#include <CST816S.h>
#include <FunctionalInterrupt.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <functional>
#include <lvgl.h>
#include <ui.h>

// XIAOの標準I2Cピンとタッチパネル用ピン
#define TOUCH_SDA D4
#define TOUCH_SCL D5
#define TOUCH_INT D7 // ディスプレイの仕様に合わせて確認してください
#define TOUCH_RST D6 // ディスプレイの仕様に合わせて確認してください

// Backlight Pin (User confirmed D2)
#define BACKLIGHT_PIN D2

// Backlight Polarity (Change to LOW if inverted)
#define BL_ON HIGH
#define BL_OFF LOW

CST816S touch(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);

/*Don't forget to set Sketchbook location in File/Preferences to the path of
 * your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 280;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[screenWidth * screenHeight];
// static lv_color_t buf2[screenWidth * screenHeight / 2];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

// Backlight State
static bool is_display_on = true;
static uint32_t last_touch_time = 0;
static uint32_t display_wake_time = 0; // Tracks when display turned ON
static int target_brightness = 255;    // Default max brightness
static int current_brightness = 255;

// Helper to set PWM directly
void set_raw_brightness(int val) {
  if (BL_ON == LOW)
    val = 255 - val; // Invert control if needed
  analogWrite(BACKLIGHT_PIN, val);

  // Store localized brightness (regardless of hardware inversion)
  if (BL_ON == LOW)
    current_brightness = 255 - val;
  else
    current_brightness = val;
}

// Helper to fade backlight
void fade_backlight(bool in) {
  int start = current_brightness;
  int end = in ? target_brightness : 0;
  int step = (start < end) ? 10 : -10; // Faster fade (snappy)

  // Simple blocking fade
  while (abs(current_brightness - end) > 10) {
    current_brightness += step;
    set_raw_brightness(current_brightness);
    delay(5);
  }
  set_raw_brightness(end);
}

// Function called from UI to update brightness setting
void update_user_brightness(int val) {
  target_brightness = val;
  if (is_display_on) {
    set_raw_brightness(target_brightness);
  }
}

void set_display_state(bool on) {
  if (is_display_on == on)
    return;
  is_display_on = on;

  if (on) {
    display_wake_time = millis(); // Record wake time
    tft.writecommand(0x11);       // Sleep Out
    tft.writecommand(0x29);       // Display ON
    delay(10);                    // Wait for TFT to wake
    fade_backlight(true);         // Fade IN
  } else {
    fade_backlight(false);  // Fade OUT
    tft.writecommand(0x28); // Display OFF
    tft.writecommand(0x10); // Sleep In
  }
}

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
#include <SPI.h> // Ensure SPI is available

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);

  // Direct SPI DMA Transfer (Chunked for nRF52 limit < 65535)
  uint32_t total_bytes = w * h * 2;
  uint8_t *data_ptr = (uint8_t *)&color_p->full;
  const uint32_t chunk_size = 60000; // Safe chunk size under 64KB

  while (total_bytes > 0) {
    uint32_t transfer_size =
        (total_bytes > chunk_size) ? chunk_size : total_bytes;
    SPI.transfer(data_ptr, transfer_size);
    data_ptr += transfer_size;
    total_bytes -= transfer_size;
  }

  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  // Serial.println("Reading touchpad..."); // デバッグ用 (Performance Impact)
  if (touch.available()) {
    data->state = LV_INDEV_STATE_PR; // 押されている状態
    data->point.x = touch.data.x;
    data->point.y = touch.data.y;

    // Activity detected
    last_touch_time = millis();
    if (!is_display_on) {
      set_display_state(true); // Wake up immediately
    }

    // デバッグ用
    // Serial.printf("Touch: x=%d, y=%d\n", touch.data.x, touch.data.y);
  } else {
    data->state = LV_INDEV_STATE_REL; // 離されている状態
  }
}

void setup() {
  Serial.begin(115200);
  delay(100); // シリアルと電源の安定待ち

  pinMode(D4, INPUT_PULLUP);
  pinMode(D5, INPUT_PULLUP);

  // Backlight Init (PWM)
  pinMode(BACKLIGHT_PIN, OUTPUT);
  set_raw_brightness(target_brightness); // Start ON
  last_touch_time = millis();
  display_wake_time = millis(); // Initial wake time

  delay(10);
  Wire.begin();
  Wire.setClock(100000); // 100kHz（標準速度）で開始
  touch.begin();         // その後にタッチを初期化
  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() +
                  "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");

  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(
      my_print); /* register print function for debugging */
#endif

  tft.begin();             /* TFT init */
  tft.setSwapBytes(false); // これでLVGLの代わりに色を正しく整えます
  // tft.invertDisplay(false); // 円形ディスプレイは色が反転しやすいため必須
  tft.setRotation(0); /* Landscape orientation, flipped */

  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, screenWidth * screenHeight);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

#include "my_ui.h" // Add custom UI header

  // ... (existing code) ...

  // ui_init();      // Comment out old UI
  my_ui_init(); // Initialize new Swipe UI & Clock

  Serial.println("Setup done");
  last_touch_time = millis(); // Initialize timer

  // I2Cピンを明示的に入力プルアップに設定（Wire.beginの前に実行）

  // // I2Cスキャナー（setup内に追加）
  // byte error, address;
  // Serial.println("Scanning I2C...");
  // for (address = 1; address < 127; address++)
  // {
  //   Wire.beginTransmission(address);
  //   error = Wire.endTransmission();
  //   if (error == 0)
  //   {
  //     Serial.printf("I2C device found at address 0x%02X\n", address);
  //   }
  // }
  // Serial.println("I2C scan complete");
}

void loop() {
  static uint32_t lastTick = 0;
  uint32_t current = millis();
  lv_tick_inc(current - lastTick);
  lastTick = current;

  lv_timer_handler(); /* let the GUI do its work */

  // Auto Display Off Logic
  if (is_display_on) {
    // Turn off IF inactivity > 10s AND minimum ON duration > 10s
    if ((current - last_touch_time > 10000) &&
        (current - display_wake_time > 10000)) {
      set_display_state(false);
    }
  }

  delay(5);
}
