/**
 * lvgl_setup.h — LVGL 初始化与驱动接口
 */
#pragma once

#include <Arduino.h>

/**
 * 初始化 LVGL 内核、显示驱动、触摸驱动。
 * 在 setup() 中调用一次。
 */
void lvgl_setup_init(void);

/**
 * LVGL 主循环：驱动定时器和渲染。
 * 在 loop() 中反复调用。
 */
void lvgl_setup_loop(void);
