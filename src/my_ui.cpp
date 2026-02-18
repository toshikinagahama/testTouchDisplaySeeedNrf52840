#include "my_ui.h"
#include <math.h>

// UI Objects
static lv_obj_t *tv; // Tileview
static lv_obj_t *lbl_hr;
static lv_obj_t *lbl_batt;
static lv_obj_t *lbl_steps;

// Clock Hands
static lv_obj_t *hour_hand;
static lv_obj_t *min_hand;
static lv_obj_t *sec_hand;

// Clock center and radius
#define CLOCK_CX 120
#define CLOCK_CY 140
#define CLOCK_R 100

// Static point arrays for lines
static lv_point_t hour_points[2];
static lv_point_t min_points[2];
static lv_point_t sec_points[2];

// Simulation Data
static int val_hr = 72;
static int val_steps = 1000;
static int val_batt = 100;

// Navigation Event Callback
static void dashboard_nav_cb(lv_event_t *e) {
  intptr_t target = (intptr_t)lv_event_get_user_data(e);

  if (target == 0)
    lv_obj_set_tile_id(tv, 0, 1, LV_ANIM_ON); // Battery -> Left
  else if (target == 1)
    lv_obj_set_tile_id(tv, 2, 1, LV_ANIM_ON); // HR -> Right
  else if (target == 2)
    lv_obj_set_tile_id(tv, 1, 2, LV_ANIM_ON); // Steps -> Bottom
}

// Helper to set line points based on angle
static void update_hand_position(lv_obj_t *line, lv_point_t *points,
                                 float angle_deg, int length) {
  // Invalidate the OLD position before updating points
  lv_obj_invalidate(line);

  float angle_rad = (angle_deg - 90.0f) * 3.14159f / 180.0f;
  points[0].x = CLOCK_CX;
  points[0].y = CLOCK_CY;
  points[1].x = CLOCK_CX + (int)(cos(angle_rad) * length);
  points[1].y = CLOCK_CY + (int)(sin(angle_rad) * length);

  lv_line_set_points(line, points, 2);

  // Invalidate the NEW position to ensure it gets drawn
  lv_obj_invalidate(line);
}

// Timer callback to update clock and data
static void clock_timer_cb(lv_timer_t *timer) {
  // 1. Update Clock (Start at 10:10:00)
  uint32_t start_offset = (10 * 3600 + 10 * 60) * 1000;
  uint32_t t = millis() + start_offset;

  float s = (t / 1000) % 60;
  float m = ((t / 1000) / 60) % 60;
  float h = ((t / 1000) / 3600) % 12;

  update_hand_position(sec_hand, sec_points, s * 6, CLOCK_R - 10);
  update_hand_position(min_hand, min_points, m * 6 + s * 0.1f, CLOCK_R - 20);
  update_hand_position(hour_hand, hour_points, h * 30 + m * 0.5f, CLOCK_R - 40);

  // 2. Update Simulation Data (Every 1 second approx)
  static uint32_t last_sec = 0;
  if (t / 1000 != last_sec) {
    last_sec = t / 1000;

    // HR: Random 60-100
    val_hr = 60 + (rand() % 41);
    lv_label_set_text_fmt(lbl_hr, "#FF0000 \xEF\x80\x84# %d", val_hr);

    // Steps: Increment
    val_steps++;
    lv_label_set_text_fmt(lbl_steps, "#00FFFF \xEF\x95\x8B# %d", val_steps);

    // Battery: Decrement every 10 sec
    if (last_sec % 10 == 0 && val_batt > 0)
      val_batt--;
    lv_label_set_text_fmt(lbl_batt, "#00FF00 \xEF\x89\x80# %d%%", val_batt);
  }
}

// Helper to create a data widget
static void create_data_widget(lv_obj_t *parent, int x, int y, int w, int h,
                               intptr_t nav_target, lv_obj_t **lbl_store) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0); // Transparent background
  lv_obj_set_style_shadow_width(btn, 0, 0);       // No shadow
  lv_obj_add_event_cb(btn, dashboard_nav_cb, LV_EVENT_CLICKED,
                      (void *)nav_target);

  lv_obj_t *lbl = lv_label_create(btn);
  lv_obj_center(lbl);
  lv_label_set_recolor(lbl, true); // Enable color codes
  lv_label_set_text(lbl, "--");
  *lbl_store = lbl;
}

static void create_dashboard(lv_obj_t *parent) {
  // 1. Analog Clock Face
  lv_obj_t *face = lv_obj_create(parent);
  lv_obj_set_size(face, CLOCK_R * 2, CLOCK_R * 2);
  lv_obj_center(face);
  lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(face, lv_color_hex(0x202020), 0);
  lv_obj_clear_flag(face, LV_OBJ_FLAG_SCROLLABLE);

  // 2. Data Widgets
  create_data_widget(parent, 10, 10, 80, 40, 1, &lbl_hr);
  create_data_widget(parent, 150, 10, 80, 40, 0, &lbl_batt);
  create_data_widget(parent, 70, 230, 100, 40, 2, &lbl_steps);

  // 3. Hands
  static lv_style_t style_thick;
  lv_style_init(&style_thick);
  lv_style_set_line_width(&style_thick, 6);
  lv_style_set_line_rounded(&style_thick, true);

  static lv_style_t style_thin;
  lv_style_init(&style_thin);
  lv_style_set_line_width(&style_thin, 3);
  lv_style_set_line_rounded(&style_thin, true);

  hour_hand = lv_line_create(parent);
  lv_obj_add_style(hour_hand, &style_thick, 0);
  lv_obj_set_style_line_color(hour_hand, lv_color_white(), 0);
  lv_line_set_points(hour_hand, hour_points, 2);

  min_hand = lv_line_create(parent);
  lv_obj_add_style(min_hand, &style_thick, 0);
  lv_obj_set_style_line_color(min_hand, lv_color_hex(0xAAAAAA), 0);
  lv_line_set_points(min_hand, min_points, 2);

  sec_hand = lv_line_create(parent);
  lv_obj_add_style(sec_hand, &style_thin, 0);
  lv_obj_set_style_line_color(sec_hand, lv_palette_main(LV_PALETTE_RED), 0);
  lv_line_set_points(sec_hand, sec_points, 2);

  update_hand_position(hour_hand, hour_points, 0, CLOCK_R - 40);
  update_hand_position(min_hand, min_points, 0, CLOCK_R - 20);
  update_hand_position(sec_hand, sec_points, 0, CLOCK_R - 10);

  lv_timer_create(clock_timer_cb, 50, NULL);
}

// Slider Event Callback
static void slider_event_cb(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  int val = (int)lv_slider_get_value(slider);

  // Call main.cpp function to update brightness
  update_user_brightness(val);
}

static void create_settings_screen(lv_obj_t *parent) {
  // Background
  lv_obj_set_style_bg_color(parent, lv_color_hex(0x202020), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

  // Title "Settings"
  lv_obj_t *title = lv_label_create(parent);
  lv_label_set_text(title, "Settings");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  // Brightness Slider
  lv_obj_t *slider = lv_slider_create(parent);
  lv_obj_set_width(slider, 180);
  lv_obj_center(slider);
  lv_slider_set_range(slider, 10, 255);          // Min 10 to prevent blackout
  lv_slider_set_value(slider, 255, LV_ANIM_OFF); // Default max
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // Label "Brightness"
  lv_obj_t *lbl = lv_label_create(parent);
  lv_label_set_text(lbl, "Backlight Brightness");
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align_to(lbl, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);

  // Icon
  lv_obj_t *icon = lv_label_create(parent);
  lv_label_set_text(icon, "\xEF\x80\x84"); // Use heart or sun icon if available
  lv_obj_align_to(icon, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void my_ui_init(void) {
  tv = lv_tileview_create(lv_scr_act());
  lv_obj_set_style_bg_color(tv, lv_color_black(), 0);

  // Tile 1: Center (Dashboard)
  lv_obj_t *tile_center = lv_tileview_add_tile(tv, 1, 1, LV_DIR_ALL);
  create_dashboard(tile_center);

  // Tile 2: Top (Settings)
  lv_obj_t *tile_top = lv_tileview_add_tile(tv, 1, 0, LV_DIR_BOTTOM);
  create_settings_screen(tile_top);

  // Tile 3: Bottom (Steps Details)
  lv_obj_t *tile_bottom = lv_tileview_add_tile(tv, 1, 2, LV_DIR_TOP);
  lv_obj_t *lbl = lv_label_create(tile_bottom);
  lv_label_set_text(lbl, "Steps Details\n\n- Today: 12345\n- Goal: 10000");
  lv_obj_center(lbl);
  lv_obj_set_style_bg_color(tile_bottom, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_bg_opa(tile_bottom, LV_OPA_COVER, 0);

  // Tile 4: Left (Battery Details)
  lv_obj_t *tile_left = lv_tileview_add_tile(tv, 0, 1, LV_DIR_RIGHT);
  lbl = lv_label_create(tile_left);
  lv_label_set_text(lbl, "Battery Status\n\n- Level: 85%\n- Charging: No");
  lv_obj_center(lbl);
  lv_obj_set_style_bg_color(tile_left, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_set_style_bg_opa(tile_left, LV_OPA_COVER, 0);

  // Tile 5: Right (HR Details)
  lv_obj_t *tile_right = lv_tileview_add_tile(tv, 2, 1, LV_DIR_LEFT);
  lbl = lv_label_create(tile_right);
  lv_label_set_text(lbl, "Heart Rate\n\n- Avg: 72 bpm\n- Max: 120 bpm");
  lv_obj_center(lbl);
  lv_obj_set_style_bg_color(tile_right, lv_palette_main(LV_PALETTE_PURPLE), 0);
  lv_obj_set_style_bg_opa(tile_right, LV_OPA_COVER, 0);

  // Initial Tile
  lv_obj_set_tile(tv, tile_center, LV_ANIM_OFF);
}
