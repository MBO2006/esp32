/**
 * display.h — 显示辅助函数接口
 */
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * 获取全局 TFT 对象（供外部使用）
 */
TFT_eSPI& getTft();

/**
 * 初始化显示屏，在 setup() 中调用
 */
void displayInit();

/**
 * 在屏幕上显示触摸原始值
 * @param rawX X 原始值
 * @param rawY Y 原始值
 * @param z    压力值
 * @param touched 是否正在触摸
 */
void displayTouchInfo(uint16_t rawX, uint16_t rawY, uint16_t z, bool touched);
