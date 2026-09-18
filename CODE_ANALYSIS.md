# 代码深度解析

本文档逐行解析 ESP32-S3 + ILI9341 + XPT2046 触摸屏项目的每个模块，适合嵌入式开发学习。

---

## 目录

1. [硬件背景](#1-硬件背景)
2. [项目架构总览](#2-项目架构总览)
3. [config.h — 配置中心](#3-configh--配置中心)
4. [touch.h / touch.cpp — 触摸驱动](#4-touchh--touchcpp--触摸驱动)
5. [chinese_font.h — 中文字库](#5-chinese_fonth--中文字库)
6. [display.h / display.cpp — 显示模块](#6-displayh--displaycpp--显示模块)
7. [loop.h / loop.cpp — 主循环与页面逻辑](#7-looph--loopcpp--主循环与页面逻辑)
8. [Time.h / Time.cpp — 计时器模块](#8-timeh--timecpp--计时器模块)
9. [led.h / led.cpp — LED 控制模块](#9-ledh--ledcpp--led-控制模块)
10. [main.cpp — 应用主程序](#10-maincpp--应用主程序)
11. [platformio.ini — 构建配置](#11-platformioini--构建配置)
12. [开发工具](#12-开发工具)
13. [踩坑全记录](#13-踩坑全记录)
14. [扩展学习](#14-扩展学习)

---

## 1. 硬件背景

### 1.1 ESP32-S3 芯片

ESP32-S3 是乐鑫推出的 Wi-Fi + BLE 5.0 MCU，主要特点：

| 特性 | 说明 |
|:---|:---|
| CPU | Xtensa LX7 双核，240MHz |
| RAM | 512KB SRAM + 外接 PSRAM（本板8MB） |
| Flash | 8MB QSPI Flash |
| GPIO | 45个可用 GPIO |
| SPI 外设 | 4个（SPI0~SPI3），但 SPI0/1 被 Flash 占用 |
| USB | 原生 USB-OTG，支持 USB CDC 串口 |

**关键限制**：SPI0 和 SPI1 连接内部 Flash/PSRAM，**绝对不能被用户代码触碰**，否则芯片崩溃。

### 1.2 ILI9341 显示屏

ILI9341 是一颗 240×320 像素的 TFT LCD 驱动芯片，通过 SPI 接口通信：

```
ESP32-S3                    ILI9341
────────                    ───────
MOSI (数据输出) ──────────→ SDI (数据输入)
MISO (数据输入) ←────────── SDO (数据输出)
SCK  (时钟)     ──────────→ SCK (时钟)
CS   (片选)     ──────────→ CS  (片选，低有效)
DC   (数据/命令) ──────────→ DC  (0=命令, 1=数据)
RST  (复位)     ──────────→ RST (低电平复位)
```

### 1.3 XPT2046 触摸控制器

XPT2046 是电阻式触摸屏的 ADC 芯片，把手指按压产生的模拟电压转换为数字坐标（0~4095），通过 bit-bang SPI 与 ESP32 通信。

---

## 2. 项目架构总览

```
esp32/
├── platformio.ini              ← 构建配置（引脚、库、编译选项）
├── CODE_ANALYSIS.md            ← 本文档
├── CHANGELOG.md                ← 版本变更记录
├── README.md                   ← 项目说明与接线教程
├── tools/
│   ├── font_converter.py       ← 字体转换脚本（命令行版）
│   ├── font_converter_gui.py   ← 字体转换 GUI 工具
│   ├── jpeg_converter.py       ← JPEG 图片转换脚本（命令行版）
│   └── jpeg_converter_gui.py   ← JPEG 图片转换 GUI 工具
└── src/
    ├── config.h                ← ① 引脚定义、全局常量
    ├── touch.h / touch.cpp     ← ② 触摸驱动（bit-bang SPI）
    ├── chinese_font.h          ← ③ 中文字库（自动生成，66个汉字）
    ├── display.h / display.cpp ← ④ 显示模块（页面绘制、按钮、JPEG）
    ├── loop.h / loop.cpp       ← ⑤ 主循环（触摸检测、页面路由）
    ├── Time.h / Time.cpp       ← ⑥ 计时器模块（秒表功能）
    ├── led.h / led.cpp         ← ⑦ LED 控制模块
    ├── img_photo.h             ← ⑧ JPEG 图片数据（自动生成）
    └── main.cpp                ← ⑨ 应用入口（setup 初始化）
```

### 模块依赖关系

```
main.cpp
  ├── config.h        (LED_PIN, SERIAL_BAUD)
  ├── touch.h/cpp     (初始化触摸)
  └── display.h/cpp   (初始化屏幕、画首页)

loop.cpp
  ├── config.h        (TOUCH_Z_THRESHOLD)
  ├── touch.h/cpp     (读触摸坐标)
  ├── display.h/cpp   (页面切换、按钮检测)
  ├── Time.h/cpp      (AddTime、startTimer、pauseTimer、resetTimer)
  └── led.h/cpp       (ledon、ledoff)

display.cpp
  ├── config.h
  ├── touch.h
  ├── chinese_font.h  (中文字模位图)
  ├── JPEGDecoder.h   (JPEG 解码库)
  ├── img_photo.h     (JPEG 图片数据)
  ├── time.h          (计时器函数)
  └── TFT_eSPI 库     (底层显示驱动)

Time.cpp
  ├── display.h       (getTft、getCurrentPage)
  └── Arduino.h

led.cpp
  ├── config.h        (LED_PIN)
  └── Arduino.h
```

---

## 3. config.h — 配置中心

```cpp
#pragma once
#include <Arduino.h>

// ===== 触摸 SPI 引脚（bit-bang，独立于显示 SPI）=====
#define T_CS   7   // 触摸芯片片选
#define T_CLK  14  // 触摸 SPI 时钟
#define T_SDI  21  // 触摸数据输入（ESP32 → XPT2046）
#define T_SDO  47  // 触摸数据输出（XPT2046 → ESP32）

// ===== 触摸参数 =====
#define TOUCH_Z_THRESHOLD  50  // 压力阈值

// ===== 串口 =====
#define SERIAL_BAUD  115200

// ===== 外设 =====
#define LED_PIN  2  // LED 引脚
```

`#define` 是编译期文本替换——没有内存开销、没有运行时开销。改引脚只改这一个文件，其他模块自动生效。

---

## 4. touch.h / touch.cpp — 触摸驱动

### 接口

```cpp
void touchInit();                                         // 初始化引脚
uint16_t xptRead(uint8_t cmd);                            // 读取 XPT2046 指定通道
void touchRead(uint16_t &rawX, uint16_t &rawY, uint16_t &z);  // 读取完整坐标
bool isTouched();                                         // 判断是否触摸
```

### 工作原理

XPT2046 使用 bit-bang SPI（手动翻转 GPIO），**独立于 TFT_eSPI 使用的硬件 SPI**，避免总线冲突。

**`touchWrite()`**：逐位发送 8-bit 数据（MSB 先发）
```
         bit7   bit6   bit5   bit4
         ┌──┐   ┌──┐   ┌──┐   ┌──┐
CLK  ────┘  └───┘  └───┘  └───┘  └───
         ╔══╗   ╔══╗
SDI  ════║1 ║═══║1 ║═══ 0  ════ 1  ════
         ╚══╝   ╚══╝
        ←2us→←2us→   一个 bit 周期 = 4us
```

**`touchRead12()`**：逐位读取 12-bit ADC 值（0~4095）

**`xptRead()`**：一次完整的 SPI 通信
```
CS   ───┘                                               └───
CLK  ───┘ └┘ └┘ └┘ └┘ └┘ └┘ └┘ └───────────────────────┘ └─
SDI  ═══║  命令 0xD0 (8 bits) ║════┘
MISO ──────────────────────────────────┤ XPT2046 返回 (12 bits)
```

**命令字节**：`0xD0` = 读 X，`0x90` = 读 Y，`0xB0` = 读压力（Z1）

**SDO 引脚为什么加 `INPUT_PULLUP`？**
当 XPT2046 没被选中时，输出引脚是高阻态（悬空），会拾取电磁干扰。内部上拉电阻把悬空状态拉到 HIGH，避免误读。

---

## 5. chinese_font.h — 中文字库

TFT_eSPI 库内置了英文字体，但**不包含中文字库**。解决方案是用 Python 工具从系统字体中提取项目实际用到的汉字，生成 PROGMEM 位图数组。

### 字库结构

```cpp
// 字体: msyh.ttc, 字号: 24px
// 字符数: 66

#define CN_FONT_SIZE 24
#define CN_CHAR_COUNT 66

// '佳' (U+4F73) 32x30
static const uint16_t cn_char_000[] PROGMEM = { 0x0000, 0x0000, ... };
```

**PROGMEM**：数据放在 Flash 而非 RAM。ESP32-S3 有 8MB Flash 但只有 320KB RAM，字库数据必须放 Flash。

### 查找表与渲染

字库文件包含 `CnCharInfo` 结构体数组和 `cnDrawString()` 函数：
1. 遍历 UTF-8 字符串，解析每个字符的 Unicode 码点
2. 在查找表中找到对应的位图数据
3. 用 TFT_eSPI 的 `pushImage()` 逐行绘制到屏幕

### 字体转换工具

| 工具 | 说明 |
|:---|:---|
| `tools/font_converter.py` | 命令行版，自动从系统字体提取 |
| `tools/font_converter_gui.py` | GUI 版，支持预览、增量更新 |

---

## 6. display.h / display.cpp — 显示模块

### 6.1 核心数据结构

```cpp
struct Button {
    uint16_t x, y, w, h;   // 按钮位置和大小
    const char *label;      // 按钮标签（支持中文 UTF-8）
};

enum Page {
    PAGE_HOME,       // 首页（菜单选择）
    PAGE_FUNCTION,   // 计时器页
    PAGE1,           // 功能入口页
    PAGE2,           // 图片展示页
    PAGE_COUNT,      // 页面总数（哨兵值）
    PAGE_DIANDENG    // LED 控制页
};
```

### 6.2 按钮声明

所有按钮的**实际定义**在 [loop.cpp](#7-looph--loopcpp--主循环与页面逻辑)，`display.h` 只放 `extern` 声明：

```cpp
// 页面
extern Button page1;
extern Button page2;
extern Button backBtn;

// 计时器
extern Button functionBtn;
extern Button jishikaishi;
extern Button jishitingzhi;
extern Button jishichongzhi;

// LED 灯
extern Button onled;
extern Button offled;
extern Button diandeng;
```

**`extern` 的含义**：告诉编译器"这个变量在别的 `.cpp` 文件里定义了，链接时再去找"。

### 6.3 按钮绘制（当前版本：纯文字）

```cpp
void drawButton(const Button &btn)
{
    // 按钮不可见，只画文字（触摸区域仍是 btn.x/y/w/h 那个矩形）
    tft.setTextFont(2);
    cnDrawString(&tft, btn.x + 10, btn.y + 5, btn.label);
}
```

按钮在屏幕上**只有文字**，没有边框和填充。触摸区域是 `btn.x/y/w/h` 定义的 100×40 矩形。

### 6.4 页面绘制

**首页 `drawPageHome()`**：
```cpp
void drawPageHome() {
    tft.fillScreen(TFT_BLACK);
    cnDrawString(&tft, 10, 10, "这是菜单");
    cnDrawString(&tft, 10, 40, "你可以选择页面并点击");
    drawButton(page1);   // "功能页"
    drawButton(page2);   // "页面2"
}
```

**功能入口页 `drawpage1()`**：
```cpp
void drawpage1() {
    tft.fillScreen(TFT_BLACK);
    drawButton(backBtn);      // "返回"
    drawButton(functionBtn);  // "计时器"
    drawButton(diandeng);     // "灯"
}
```

**计时器页 `drawPageFunction()`**：
```cpp
void drawPageFunction() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(4);
    tft.drawString("00:00:00", 10, 60);   // 初始显示时间（大号字体）
    cnDrawString(&tft, 10, 10, "计时器");
    drawButton(jishikaishi);    // "计时开始"
    drawButton(jishichongzhi);  // "计时重置"
    drawButton(jishitingzhi);   // "计时停止"
    drawButton(backBtn);        // "返回"
}
```

**图片展示页 `drawpage2()`**：
```cpp
void drawpage2() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("PAGE2", 10, 10);
    drawButton(backBtn);
    drawJpeg(img_photo, sizeof(img_photo), 0, 0);  // 显示 JPEG 图片
}
```

**LED 控制页 `drawpagediandeng()`**：
```cpp
void drawpagediandeng() {
    tft.fillScreen(TFT_BLACK);
    drawButton(onled);    // "开灯"
    drawButton(offled);   // "关灯"
    drawButton(backBtn);  // "返回"
}
```

### 6.5 页面切换函数

统一模式：**更新状态变量 + 重绘页面**。

```cpp
void switchToHome()     { currentPage = PAGE_HOME;     drawPageHome(); }
void switchToFunction() { currentPage = PAGE_FUNCTION; drawPageFunction(); }
void gotopage1()        { currentPage = PAGE1;         drawpage1(); }
void gotopage2()        { currentPage = PAGE2;         drawpage2(); }
void switchdiandeng()   { currentPage = PAGE_DIANDENG; drawpagediandeng(); }
```

### 6.6 JPEG 图片显示

`drawJpeg()` 基于 TFT_eSPI 官方示例，使用 JPEGDecoder 库逐块解码并推送到屏幕：

```cpp
void drawJpeg(const uint8_t *data, uint32_t size, int xpos, int ypos)
{
    JpegDec.decodeArray(data, size);       // ① 解码 JPEG 数据

    uint16_t mcu_w = JpegDec.MCUWidth;    // ② MCU 块尺寸（通常 16×16）
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t img_w = JpegDec.width;
    uint32_t img_h = JpegDec.height;

    // 计算边缘 MCU 的实际尺寸
    uint32_t min_w = minimum(mcu_w, img_w % mcu_w);
    uint32_t min_h = minimum(mcu_h, img_h % mcu_h);

    uint32_t max_x = xpos + img_w;
    uint32_t max_y = ypos + img_h;

    while (JpegDec.read())                 // ③ 逐块解码
    {
        uint16_t *pImg = JpegDec.pImage;

        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // 计算当前块的实际宽高（边缘裁剪）
        win_w = (mcu_x + mcu_w <= max_x) ? mcu_w : min_w;
        win_h = (mcu_y + mcu_h <= max_y) ? mcu_h : min_h;

        // 只显示在屏幕范围内的块
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
        {
            tft.startWrite();
            tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);  // ④ 设置写入窗口
            while (mcu_pixels--) {
                tft.pushColor(*pImg++);                      // ⑤ 逐像素推送
            }
            tft.endWrite();
        }
        else if ((mcu_y + win_h) >= tft.height())
            JpegDec.abort();               // 图片超出底部，停止解码
    }
}
```

**为什么不用 `pushImage()` 一次推整张图？**
JPEG 解码后的数据是逐块（MCU，通常 16×16 像素）输出的，每块只占 512 字节 RAM。如果要先缓存整张 240×320 的图（153600 字节），RAM 直接爆了。逐块推送到屏幕，只需要一块的内存。

### 6.7 屏幕触摸信息显示

```cpp
void displayTouchInfo(bool touched)
{
    if (touched)
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
    else
        tft.setTextColor(TFT_RED, TFT_BLACK);
}
```

通过 `setTextColor(fg, bg)` 的第二个参数设置背景色，新文字会**自动覆盖旧文字**，不需要手动清屏。

---

## 7. loop.h / loop.cpp — 主循环与页面逻辑

### 7.1 接口

```cpp
// loop.h
void loop();  // 主循环
```

### 7.2 按钮定义

所有按钮的**实际定义**在 `loop.cpp` 顶部：

```cpp
// 页面导航
Button caidan  = {0, 278, 100, 40, "菜单"};
Button page1   = {0, 278, 100, 40, "功能页"};
Button page2   = {120, 278, 100, 40, "页面2"};
Button backBtn = {0, 278, 100, 40, "返回"};

// 计时器
Button functionBtn   = {0, 30, 100, 40, "计时器"};
Button jishikaishi   = {0, 238, 100, 40, "计时开始"};
Button jishitingzhi  = {0, 188, 100, 40, "计时停止"};
Button jishichongzhi = {120, 188, 100, 40, "计时重置"};

// LED 灯
Button onled    = {0, 30, 100, 40, "开灯"};
Button offled   = {120, 30, 100, 40, "关灯"};
Button diandeng = {120, 30, 100, 40, "灯"};
```

### 7.3 loop() 主循环

```cpp
void loop()
{
    // ① 读取触摸
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);
    bool touched = (z > TOUCH_Z_THRESHOLD);
    displayTouchInfo(touched);

    if (touched)
    {
        // ② 坐标校准（实测两点标定）
        uint16_t screenX = map(rawX, 220, 1780, 0, 239);
        uint16_t screenY = map(rawY, 0, 1765, 0, 319);

        // ③ 串口调试输出
        Serial.printf("rawX=%d rawY=%d z=%d → screenX=%d screenY=%d\n",
                      rawX, rawY, z, screenX, screenY);

        // ④ 根据当前页面分发按钮事件
        Page currentPage = getCurrentPage();

        if (currentPage == PAGE_HOME) {
            if (isInButton(screenX, screenY, page1))        gotopage1();
            else if (isInButton(screenX, screenY, page2))   gotopage2();
        }
        if (currentPage == PAGE1) {
            if (isInButton(screenX, screenY, backBtn))       switchToHome();
            else if (isInButton(screenX, screenY, functionBtn)) switchToFunction();
            else if (isInButton(screenX, screenY, diandeng)) switchdiandeng();
        }
        if (currentPage == PAGE_FUNCTION) {
            if (isInButton(screenX, screenY, backBtn))       gotopage1();
            else if (isInButton(screenX, screenY, jishikaishi))   startTimer();
            else if (isInButton(screenX, screenY, jishitingzhi))  pauseTimer();
            else if (isInButton(screenX, screenY, jishichongzhi)) resetTimer();
        }
        if (currentPage == PAGE_DIANDENG) {
            if (isInButton(screenX, screenY, onled))  ledon();
            else if (isInButton(screenX, screenY, offled)) ledoff();
            else if (isInButton(screenX, screenY, backBtn)) gotopage1();
        }
    }
    AddTime();  // ⑤ 每帧都调用，内部自己判断 timerRunning
    delay(100); // ⑥ 100ms 循环周期
}
```

### 7.4 页面路由图

```
                     ┌─────────────┐
                     │  PAGE_HOME  │
                     │   "这是菜单"  │
                     └──┬──────┬──┘
               点"功能页"│      │点"页面2"
                        ▼      ▼
             ┌──────────┐    ┌──────────┐
             │   PAGE1  │    │   PAGE2  │
             │ "计时器"  │    │ JPEG图片  │
             │  "灯"    │    └────┬─────┘
             └──┬──┬──┬─┘         │
                │  │  │           │点"返回"
    点"计时器"  │  │  │点"灯"     │
                ▼  │  ▼           ▼
     ┌──────────┐ │ ┌──────────┐ 回到 HOME
     │FUNCTION  │ │ │ DIANDENG │
     │ 秒表计时  │ │ │ LED 控制  │
     └────┬─────┘ │ └────┬─────┘
          │       │      │
  点"返回"│       │      │点"返回"
          ▼       ▼      ▼
         回到 PAGE1     回到 PAGE1
```

### 7.5 触摸校准

**问题**：XPT2046 返回的原始值（0~4095）不等于屏幕像素（240×320）。
**解决**：通过两点标定确定实际范围，用 `map()` 线性映射。

```cpp
uint16_t screenX = map(rawX, 220, 1780, 0, 239);
uint16_t screenY = map(rawY, 0, 1765, 0, 319);
```

**校准原理**：
```
map(value, fromLow, fromHigh, toLow, toHigh)
= (value - fromLow) × (toHigh - toLow) / (fromHigh - fromLow) + toLow
```

**校准过程**（实际操作步骤）：
1. 用旧校准值烧录程序
2. 打开串口监视器
3. 精确点击**已知屏幕坐标**的按钮（如 "开灯" 中心 y=50）
4. 记录串口输出的 rawY 值
5. 用两个点解方程求出新的校准范围

```
已知点：开灯按钮中心 rawY=276 ↔ screenY=50
         返回按钮中心 rawY=1655 ↔ screenY=298
解出：screenY = rawY × 319 / 1765
即：map(rawY, 0, 1765, 0, 319)
```

---

## 8. Time.h / Time.cpp — 计时器模块

### 8.1 接口

```cpp
void AddTime();      // 每帧调用，自动计时并更新显示
void startTimer();   // 开始计时
void pauseTimer();   // 暂停计时（保留当前时间）
void resetTimer();   // 重置计时（清零并暂停）
```

### 8.2 内部状态

```cpp
static bool timerRunning = false;      // 计时器是否在运行
static unsigned long lastMillis = 0;   // 上次计时的时间戳
static int seconds = 0;
static int minutes = 0;
static int hours = 0;
```

**为什么用 `static` 局部变量？**
- 生命周期是整个程序运行期间（不像普通局部变量，函数退出就销毁）
- 只有本文件内可见（不像全局变量，会污染命名空间）
- 初始化一次后保持值，完美适合"状态"场景

### 8.3 核心逻辑

```cpp
void AddTime()
{
    if (!timerRunning)      // 没在运行就直接返回
        return;

    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= 1000)  // 每 1000ms = 1秒
    {
        lastMillis = currentMillis;
        seconds++;
        // 60秒 → 分钟进位，60分钟 → 小时进位，24小时 → 清零

        // 通过 getTft() 获取屏幕对象，更新显示
        TFT_eSPI &tft = getTft();
        char timeStr[9];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hours, minutes, seconds);
        tft.setTextSize(4);
        tft.drawString(timeStr, 10, 60);
    }
}
```

### 8.4 控制函数

```cpp
void startTimer()  { timerRunning = true; }
void pauseTimer()  { timerRunning = false; }

void resetTimer()
{
    timerRunning = false;
    seconds = 0; minutes = 0; hours = 0;
    // 立刻清掉屏幕上的时间显示
    TFT_eSPI &tft = getTft();
    tft.setTextSize(4);
    tft.drawString("00:00:00", 10, 60);
}
```

### 8.5 调用时机

`AddTime()` 在 `loop()` 的**末尾**每帧调用，不受触摸状态影响：

```cpp
// loop.cpp 末尾
AddTime();   // 每帧都调用，内部自己判断 timerRunning
delay(100);
```

这样即使不触摸屏幕，计时也能持续运行和更新显示。

---

## 9. led.h / led.cpp — LED 控制模块

### 接口

```cpp
void ledon();   // 点亮 LED
void ledoff();  // 关闭 LED
```

### 实现

```cpp
#include "config.h"

void ledon()  { digitalWrite(LED_PIN, HIGH); }
void ledoff() { digitalWrite(LED_PIN, LOW); }
```

`digitalWrite()` 控制 GPIO 输出高/低电平。`LED_PIN` 在 `config.h` 中定义为 `2`。

**注意**：ESP32 GPIO 输出是 3.3V 电平，驱动 LED 需要串联限流电阻（220Ω~1kΩ），否则可能烧 LED 或引脚。

---

## 10. main.cpp — 应用主程序

```cpp
#include <Arduino.h>
#include "config.h"
#include "touch.h"
#include "display.h"

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(2000);                  // 等待 USB CDC 枚举完成
    Serial.println("=== Touch Demo ===");
    Serial.flush();

    displayInit();                // 初始化 ILI9341
    drawPageHome();               // 画首页
    touchInit();                  // 初始化触摸引脚

    Serial.println("READY");
    Serial.flush();

    pinMode(LED_PIN, OUTPUT);     // 设置 LED 引脚为输出模式

    loop();                       // 进入主循环（不会返回）
}
```

**`loop()` 不会返回**——它内部是死循环（每 100ms 重复一次）。所以 `setup()` 中 `loop()` 之后的代码**永远不会执行**。所有初始化必须写在 `loop()` 之前。

**`delay(2000)` 的原因**：ESP32-S3 使用原生 USB CDC 作为串口，上电后需要几秒钟完成枚举。

**`pinMode(LED_PIN, OUTPUT)` vs `digitalWrite(LED_PIN, HIGH)`**：
- `pinMode` 只是把引脚**设成输出模式**，此时输出电平默认是 LOW
- 要真正点亮，还需要 `digitalWrite(LED_PIN, HIGH)`（在 `led.cpp` 的 `ledon()` 里调用）

---

## 11. platformio.ini — 构建配置

```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

lib_deps =
    bodmer/TFT_eSPI@^2.5.43           # TFT 显示库
    paulstoffregen/XPT2046_Touchscreen  # 触摸库（备用）
    bodmer/JPEGDecoder                  # JPEG 解码库

build_flags =
    -D USER_SETUP_LOADED               # 跳过 TFT_eSPI 默认配置
    -D ILI9341_DRIVER                  # 显示驱动芯片
    -D TFT_MOSI=16                     # 显示 SPI 引脚
    -D TFT_SCLK=18
    -D TFT_CS=4
    -D TFT_DC=5
    -D TFT_RST=6
    -D TFT_MISO=15
    -D TOUCH_CS=7                      # 触摸片选
    -D TOUCH_IRQ=17
    -D LOAD_GLCD                       # 加载 Font1 (8×8)
    -D LOAD_FONT2                      # 加载 Font2 (12×16)
    -D LOAD_FONT4                      # 加载 Font4 (26px)
    -D SMOOTH_FONT                     # 平滑字体渲染
    -D SPI_FREQUENCY=40000000          # 显示 SPI 40MHz
    -D SPI_TOUCH_FREQUENCY=2500000
    -D USE_HSPI_PORT                   # 使用 SPI3（必须，否则 Flash 冲突）
    -D ARDUINO_USB_CDC_ON_BOOT=1       # 启用 USB 串口
    -D LED_PIN=2                       # LED 引脚
```

**关键配置说明**：

| 配置 | 说明 |
|:---|:---|
| `USE_HSPI_PORT` | 必须。ESP32-S3 默认 SPI 引脚和 Flash 冲突，去掉会崩溃 |
| `ARDUINO_USB_CDC_ON_BOOT=1` | 必须。ESP32-S3 的 USB CDC 需要额外配置才能输出串口 |
| `JPEGDecoder` | JPEG 解码库，`drawJpeg()` 依赖它 |

---

## 12. 开发工具

### 12.1 字体转换工具

| 工具 | 路径 | 说明 |
|:---|:---|:---|
| 命令行版 | `tools/font_converter.py` | 自动从系统字体提取汉字 |
| GUI 版 | `tools/font_converter_gui.py` | 带预览、增量更新 |

**用法**：
```bash
python tools/font_converter_gui.py
```

### 12.2 图片转换工具

| 工具 | 路径 | 说明 |
|:---|:---|:---|
| 命令行版 | `tools/jpeg_converter.py` | JPEG/PNG/BMP → C 数组 |
| GUI 版 | `tools/jpeg_converter_gui.py` | 带预览、缩放、质量控制 |

**用法**：
```bash
python tools/jpeg_converter_gui.py
```

**GUI 功能**：
- 支持 JPEG、PNG、BMP、GIF 等格式（自动转换为 JPEG）
- 三种缩放模式：不缩放、适应屏幕（240×320）、自定义尺寸
- JPEG 质量滑块（10%~100%）
- 快捷预设：全屏 240×320、半屏 120×160、图标 48×48、32×32
- 生成 `.h` 文件，包含 PROGMEM 字节数组

**生成的文件格式**（`img_photo.h`）：
```cpp
// 来源: photo.png (转为JPEG) [1920×1080 → 240×135]
// 大小: 64606 字节 (63.1 KB)
// JPEG 质量: 80%
// 用法: drawJpeg(img_photo, sizeof(img_photo), x, y);

const uint8_t img_photo[] PROGMEM = { 0xFF, 0xD8, 0xFF, 0xE0, ... };
```

---

## 13. 踩坑全记录

### 坑1：GPIO 11/12/13 黑屏

**现象**：屏幕全黑，芯片无法启动
**原因**：GPIO 11/12/13 是 ESP32-S3 的 SPI Flash 接口
**教训**：先查数据手册的引脚功能表，再选 GPIO

### 坑2：ILI9341 模块 T_SDI 不连通

**现象**：触摸读到全0或固定值4095
**原因**：ILI9341 模块的 SDI(MOSI) 和 T_SDI 在 PCB 内部**没有连通**
**教训**：模块上的同类引脚不一定内部连通，需要万用表确认

### 坑3：显示和触摸共享 SPI 总线

**现象**：`getTouchRaw()` 返回全0
**解决方案**：触摸改用 bit-bang（手动翻转 GPIO），完全独立于硬件 SPI

### 坑4：USE_HSPI_PORT 去不掉

**现象**：去掉后板子崩溃（Guru Meditation Error）
**原因**：ESP32-S3 默认 SPI 引脚和 Flash 冲突
**教训**：ESP32-S3 的 SPI 配置和 ESP32 不同，不能照搬

### 坑5：字体不显示

**现象**：`drawString` 什么都不画
**解决方案**：在 `build_flags` 里加 `-D LOAD_GLCD -D LOAD_FONT2`

### 坑6：ESP32-S3 串口无输出

**现象**：`Serial.println()` 没有输出
**解决方案**：加 `-D ARDUINO_USB_CDC_ON_BOOT=1`

### 坑7：触摸坐标上方偏差大

**现象**：屏幕上方按钮要往下点才触发，下方按钮正常
**原因**：校准参数不准，`map(rawY, 200, 1830, 0, 319)` 的顶部锚点（200）偏大
**排查方法**：
1. 串口打印原始值
2. 精确点击已知坐标的按钮（如 "开灯" 中心 y=50）
3. 记录 rawY，解方程求正确范围
**解决方案**：`map(rawY, 0, 1765, 0, 319)`（实测两点标定）

### 坑8：按钮不可见，触摸靠猜

**现象**：按钮画在黑色背景上，完全看不见
**原因**：`drawButton()` 用 `TFT_BLACK` 画边框和填充
**解决方案**：调试时改成蓝色底+白色边框，调好后改为纯文字（不可见按钮）

### 坑9：`pinMode` 不会点亮 LED

**现象**：`pinMode(LED, HIGH)` 不亮
**原因**：`pinMode` 第二个参数是模式（OUTPUT/INPUT），不是电平。`HIGH` 在 ESP32 里恰好等于 `INPUT`，所以引脚被设成了输入模式
**解决方案**：`pinMode(LED_PIN, OUTPUT);` + `digitalWrite(LED_PIN, HIGH);`

### 坑10：头文件里写变量定义

**现象**：`multiple definition of 'page1'`
**原因**：把 `Button page1 = {...};` 写在 `.h` 头文件里，被多个 `.cpp` include 后出现重复定义
**解决方案**：`.h` 只放 `extern` 声明，`.cpp` 放实际定义

### 坑11：`void AddTime();` 当成调用

**现象**：函数不执行
**原因**：`void AddTime();` 是声明，不是调用。调用应该写 `AddTime();`
**教训**：声明告诉编译器"有这个函数"，调用才真正执行它

### 坑12：PlatformIO 迁移后编译失败

**现象**：从 C 盘迁移到 D 盘后报错
**解决方案**：设置 `PLATFORMIO_CORE_DIR=D:\PlatformIO`，重新安装 `penv`

---

## 14. 扩展学习

### 14.1 已实现的功能

- [x] 触摸读取与串口输出
- [x] 触摸坐标校准（两点标定 + map 映射）
- [x] 中文字库（msyh 24px，66个汉字）
- [x] 多页面系统（首页 → 功能入口 → 计时器/LED/图片展示）
- [x] 不可见按钮导航（纯文字，触摸区域隐形）
- [x] 秒表计时器（开始/暂停/重置）
- [x] LED 开关控制
- [x] JPEG 图片显示（JPEGDecoder 解码 + MCU 逐块渲染）
- [x] 图片转换工具（命令行 + GUI，支持 PNG/BMP 缩放）
- [x] 字体转换工具（命令行 + GUI，支持增量更新）
- [x] 代码模块化（config/touch/display/loop/time/led/main）

### 14.2 下一步改进方向

1. **按钮按下反馈**：触摸时变色，松开恢复
2. **防抖处理**：加时间间隔，避免一次触摸触发多次
3. **过渡动画**：页面切换时的淡入淡出效果
4. **中断驱动触摸**：用 T_IRQ 引脚触发中断，避免轮询
5. **手环 UI**：圆形/弧形界面适配小屏幕
6. **NTP 时间同步**：通过 Wi-Fi 获取真实时间

### 14.3 关键概念速查

| 概念 | 解释 |
|:---|:---|
| SPI | 同步串行协议，4线（CLK/MOSI/MISO/CS） |
| Bit-bang | 手动翻转 GPIO 实现协议，不用硬件 SPI |
| PROGMEM | 数据放在 Flash 而非 RAM 中 |
| RGB565 | 16-bit 颜色格式：5位红+6位绿+5位蓝 |
| MCU | Minimum Coding Unit，JPEG 解码的最小单元（通常 16×16） |
| `map()` | Arduino 线性映射函数 |
| `extern` | 声明变量在其他文件中定义 |
| `#define` | 编译期文本替换，不占内存 |
| `static` 局部变量 | 生命周期是整个程序，但只在本文件可见 |
| `digitalWrite()` | 控制 GPIO 输出高/低电平 |
| `pinMode()` | 设置引脚模式（INPUT/OUTPUT），**不是**设置电平 |
| USB CDC | USB 虚拟串口，ESP32-S3 需要额外配置 |
