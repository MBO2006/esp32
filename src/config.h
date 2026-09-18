/**
 * config.h — 引脚定义与全局常量
 *
 * 修改引脚只需改这个文件，其他模块自动生效。
 */
#pragma once

#include <Arduino.h>

// ===== 触摸 SPI 引脚（bit-bang，独立于显示 SPI）=====
#define T_CS 7   // 触摸芯片片选
#define T_CLK 14 // 触摸 SPI 时钟
#define T_SDI 21 // 触摸数据输入（ESP32 → XPT2046）
#define T_SDO 47 // 触摸数据输出（XPT2046 → ESP32）

// ===== 显示引脚在 platformio.ini 的 build_flags 中配置 =====
// TFT_MOSI=16, TFT_SCLK=18, TFT_CS=4, TFT_DC=5, TFT_RST=6, TFT_MISO=15

// ===== 触摸参数 =====
#define TOUCH_Z_THRESHOLD 50 // 压力阈值，低于此值视为未触摸

// ===== 串口 =====
#define SERIAL_BAUD 115200
#define LED_PIN 2
