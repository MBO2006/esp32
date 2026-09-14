/**
 * display.h — 显示辅助函数接口
 */
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * 按钮结构体
 */
struct Button
{
    uint16_t x, y, w, h; // 按钮位置和大小
    const char *label;   // 按钮标签
};

/**
 * 获取全局 TFT 对象（供外部使用）
 */
TFT_eSPI &getTft();

/**
 * 初始化显示屏，在 setup() 中调用
 */
void displayInit();

/**
 * 在屏幕上显示触摸原始值
 */
void displayTouchInfo(uint16_t rawX, uint16_t rawY, uint16_t z, bool touched);

/**
 * 清屏并重绘标题
 */
void displayClean();

/**
 * 绘制按钮
 */
void drawButton(const Button &btn);

/**
 * 判断触摸点是否在按钮内
 */
bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn);
