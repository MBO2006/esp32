/**
 * touch.h — XPT2046 电阻触摸驱动接口
 */
#pragma once

#include <Arduino.h>

/**
 * 初始化触摸引脚（在 setup() 中调用一次）
 */
void touchInit();

/**
 * 读取 XPT2046 指定通道的 12-bit 原始值
 * @param cmd 命令字节：0xD0=X, 0x90=Y, 0xB0=Z1
 * @return 0~4095 的原始 ADC 值
 */
uint16_t xptRead(uint8_t cmd);

/**
 * 读取触摸坐标和压力
 * @param rawX 输出：X 方向原始值
 * @param rawY 输出：Y 方向原始值
 * @param z    输出：压力值（> TOUCH_Z_THRESHOLD 表示触摸中）
 */
void touchRead(uint16_t &rawX, uint16_t &rawY, uint16_t &z);

/**
 * 判断是否正在触摸
 * @return true = 有触摸，false = 无触摸
 */
bool isTouched();
