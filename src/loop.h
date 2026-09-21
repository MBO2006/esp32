/**
 * loop.h — 应用初始化回调接口
 */
#pragma once

/**
 * 注册 LVGL 回调（LED 控制、计时器等）
 * 在 setup() 中 screens_init() 之后调用。
 */
void app_init_callbacks(void);
