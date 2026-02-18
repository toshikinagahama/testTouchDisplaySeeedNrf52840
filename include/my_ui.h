#ifndef MY_UI_H
#define MY_UI_H

#include <Arduino.h>
#include <lvgl.h>

void my_ui_init(void);

// Call these from UI
extern void update_user_brightness(int val);

#endif
