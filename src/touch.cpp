/**
 * touch.cpp — XPT2046 电阻触摸驱动实现
 *
 * 使用 bit-bang（手动翻转 GPIO）实现 SPI 通信，
 * 独立于 TFT_eSPI 使用的硬件 SPI，避免总线冲突。
 */
#include "touch.h"
#include "config.h"
#include "display.h"

// ===== 内部函数 =====

/**
 * 发送 8-bit 数据到 XPT2046（MSB 先发）
 *
 * SPI MODE0：数据在时钟上升沿采样
 *   ┌───┐   ┌───┐   ┌───┐
 *   │   │   │   │   │   │
 * ──┘   └───┘   └───┘   └───  CLK
 *   ╔═══╗   ╔═══╗   ╔═══╗
 *   ║D7 ║   ║D6 ║   ║D5 ║     MOSI
 *   ╚═══╝   ╚═══╝   ╚═══╝
 */
static void touchWrite(uint8_t data)
{
    for (int i = 7; i >= 0; i--)
    {
        digitalWrite(T_SDI, (data >> i) & 1);
        digitalWrite(T_CLK, HIGH);
        delayMicroseconds(2);
        digitalWrite(T_CLK, LOW);
        delayMicroseconds(2);
    }
}

/**
 * 从 XPT2046 读取 12-bit 数据（MSB 先收）
 */
static uint16_t touchRead12()
{
    uint16_t val = 0;
    for (int i = 11; i >= 0; i--)
    {
        digitalWrite(T_CLK, HIGH);
        delayMicroseconds(2);
        val |= (digitalRead(T_SDO) << i);
        digitalWrite(T_CLK, LOW);
        delayMicroseconds(2);
    }
    return val;
}
// ===== 公开接口 =====

void touchInit()
{
    pinMode(T_CS, OUTPUT);
    digitalWrite(T_CS, HIGH); // 默认不选中
    pinMode(T_CLK, OUTPUT);
    digitalWrite(T_CLK, LOW); // 时钟默认低
    pinMode(T_SDI, OUTPUT);
    digitalWrite(T_SDI, LOW);     // 数据默认低
    pinMode(T_SDO, INPUT_PULLUP); // MISO 加上拉，防止悬空
}

uint16_t xptRead(uint8_t cmd)
{
    digitalWrite(T_CS, LOW);      // 选中 XPT2046
    touchWrite(cmd);              // 发送命令
    delayMicroseconds(100);       // 等待 ADC 转换
    uint16_t val = touchRead12(); // 读取结果
    digitalWrite(T_CS, HIGH);     // 释放
    return val;
}

void touchRead(uint16_t &rawX, uint16_t &rawY, uint16_t &z)
{
    rawX = xptRead(0xD0); // 读 X
    rawY = xptRead(0x90); // 读 Y
    z = xptRead(0xB0);    // 读 Z1（压力）
}

bool isTouched()
{
    return xptRead(0xB0) > TOUCH_Z_THRESHOLD;
}
