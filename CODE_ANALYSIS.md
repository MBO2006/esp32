# 代码深度解析

本文档逐行解析 ESP32-S3 + ILI9341 + XPT2046 触摸屏项目的每个模块，适合嵌入式开发学习。

---

## 目录

1. [硬件背景](#1-硬件背景)
2. [项目架构总览](#2-项目架构总览)
3. [config.h — 配置中心](#3-configh--配置中心)
4. [touch.h / touch.cpp — 触摸驱动](#4-touchh--touchcpp--触摸驱动)
5. [display.h / display.cpp — 显示模块](#5-displayh--displaycpp--显示模块)
6. [main.cpp — 应用主程序](#6-maincpp--应用主程序)
7. [platformio.ini — 构建配置](#7-platformioini--构建配置)
8. [踩坑全记录](#8-踩坑全记录)
9. [扩展学习](#9-扩展学习)

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

**DC 引脚的作用**：
- DC=LOW：发送的是**命令**（如设置显示方向、填充颜色）
- DC=HIGH：发送的是**数据**（如像素颜色值）

### 1.3 XPT2046 触摸控制器

XPT2046 是电阻式触摸屏的 ADC 芯片，把手指按压产生的模拟电压转换为数字坐标：

```
触摸屏面板                    XPT2046                   ESP32-S3
──────────                    ────────                  ────────
X+ ←──────────────────────→ XP ──→ ADC ──→ SPI ────→ T_SDO (数据输出)
X- ←──────────────────────→ XN                              ↑
Y+ ←──────────────────────→ YP                              │
Y- ←──────────────────────→ YN ←─────────────────── T_SDI (数据输入)
                              ↑
                              CS ←─────────────────── T_CS  (片选)
                              CLK ←────────────────── T_CLK (时钟)
                              IRQ ←────────────────── T_IRQ (中断)
```

**工作原理**：
1. 手指触摸屏幕 → 上下两层导电膜接触 → 产生 X 和 Y 方向的电压
2. XPT2046 内部 ADC 采集电压 → 转换为12-bit 数字值（0~4095）
3. ESP32 通过 SPI 读取这个值 → 映射为屏幕坐标

---

## 2. 项目架构总览

```
esp32_test/
├── platformio.ini          ← 构建配置（引脚、库、编译选项）
├── CHANGELOG.md            ← 版本记录（可回滚）
├── CODE_ANALYSIS.md        ← 本文档
└── src/
    ├── config.h            ← ① 引脚定义、全局常量
    ├── touch.h             ← ② 触摸驱动接口声明
    ├── touch.cpp           ← ② 触摸驱动实现
    ├── display.h           ← ③ 显示模块接口声明
    ├── display.cpp         ← ③ 显示模块实现
    └── main.cpp            ← ④ 应用主程序
```

### 模块依赖关系

```
main.cpp
  ├── config.h      (引脚常量)
  ├── touch.h/cpp   (读触摸)
  └── display.h/cpp (画屏幕)
        └── TFT_eSPI 库 (底层显示驱动)

touch.cpp
  └── config.h      (引脚常量)

display.cpp
  └── TFT_eSPI 库
```

**设计原则**：每个模块只做一件事。改引脚只动 `config.h`，改触摸逻辑只动 `touch.cpp`，改界面只动 `display.cpp`。

---

## 3. config.h — 配置中心

```cpp
#pragma once                    // 防止头文件被重复包含

#include <Arduino.h>            // 引入 Arduino 基础类型（uint8_t 等）

// ===== 触摸 SPI 引脚（bit-bang，独立于显示 SPI）=====
#define T_CS   7                // 触摸芯片片选（Chip Select）
#define T_CLK  14               // 触摸 SPI 时钟（Clock）
#define T_SDI  21               // 触摸数据输入（Serial Data In：ESP32 → XPT2046）
#define T_SDO  47               // 触摸数据输出（Serial Data Out：XPT2046 → ESP32）

// ===== 触摸参数 =====
#define TOUCH_Z_THRESHOLD  50   // 压力阈值，低于此值视为未触摸

// ===== 串口 =====
#define SERIAL_BAUD  115200     // 串口波特率
```

### 逐行解析

**`#pragma once`**
```cpp
#pragma once
```
这是"包含守卫"。C/C++ 中 `#include` 是文本复制粘贴，如果 `main.cpp` 和 `touch.cpp` 都 `#include "config.h"`，没有守卫的话 `#define T_CS 7` 会被定义两次，编译器报"重复定义"错误。`#pragma once` 确保这个文件只被包含一次。

**`#define` 宏定义**
```cpp
#define T_CS   7
```
这是**编译期文本替换**。编译器在编译前会把代码中所有 `T_CS` 替换为 `7`。它不是变量——没有内存开销，没有运行时开销。

**为什么触摸引脚选 GPIO 21/47/14/7？**
- 这些 GPIO 在 ESP32-S3-DevKitC-1 上**确实在排针上引出**
- 不与 Flash（GPIO 26-32）或 PSRAM（GPIO 33-37）冲突
- 不是启动模式相关的 strapping pin（GPIO 0、3、45、46）

---

## 4. touch.h / touch.cpp — 触摸驱动

### 4.1 头文件 touch.h

```cpp
#pragma once
#include <Arduino.h>

void touchInit();                           // 初始化触摸引脚
uint16_t xptRead(uint8_t cmd);              // 读取 XPT2046 指定通道
void touchRead(uint16_t &rawX, uint16_t &rawY, uint16_t &z);  // 读取完整坐标
bool isTouched();                           // 判断是否触摸
```

**函数声明 vs 定义**：
- `.h` 文件放**声明**（告诉编译器"有这个函数，参数是什么，返回什么"）
- `.cpp` 文件放**定义**（函数的具体实现）
- 其他文件 `#include "touch.h"` 就能调用这些函数，不需要知道实现细节

**引用参数 `&rawX`**：
```cpp
void touchRead(uint16_t &rawX, uint16_t &rawY, uint16_t &z);
```
`&` 表示引用传递——函数内部修改 `rawX` 时，调用者的变量也会被修改。这比用指针更安全，比返回结构体更简洁。

### 4.2 实现文件 touch.cpp

#### 4.2.1 头部

```cpp
#include "touch.h"    // 引入自己的接口声明
#include "config.h"   // 引入引脚定义
```

#### 4.2.2 touchWrite() — 发送8位数据

```cpp
static void touchWrite(uint8_t data) {
    for (int i = 7; i >= 0; i--) {
        digitalWrite(T_SDI, (data >> i) & 1);
        digitalWrite(T_CLK, HIGH);
        delayMicroseconds(2);
        digitalWrite(T_CLK, LOW);
        delayMicroseconds(2);
    }
}
```

**`static` 关键字**：表示这个函数只在本文件内可见，外部代码不能调用它。这是"内部实现细节"，不暴露给其他模块。

**逐位发送过程**（以 `data = 0xD0 = 11010000` 为例）：

```
循环  i   data >> i   & 1   T_SDI   说明
──────────────────────────────────────────────
 1    7   11010000    0     0       发送 bit7 = 1... 等等

让我重新算：
0xD0 = 1101 0000

data >> 7 = 0000 0001  → & 1 = 1  → T_SDI = 1  (bit7)
data >> 6 = 0000 0011  → & 1 = 1  → T_SDI = 1  (bit6)
data >> 5 = 0000 0110  → & 1 = 0  → T_SDI = 0  (bit5)
data >> 4 = 0000 1101  → & 1 = 1  → T_SDI = 1  (bit4)
data >> 3 = 0001 1010  → & 1 = 0  → T_SDI = 0  (bit3)
data >> 2 = 0011 0100  → & 1 = 0  → T_SDI = 0  (bit2)
data >> 1 = 0110 1000  → & 1 = 0  → T_SDI = 0  (bit1)
data >> 0 = 1101 0000  → & 1 = 0  → T_SDI = 0  (bit0)
```

**时序图**（每个 bit 的发送过程）：

```
         bit7   bit6   bit5   bit4
         ┌──┐   ┌──┐   ┌──┐   ┌──┐
CLK  ────┘  └───┘  └───┘  └───┘  └───
         ╔══╗   ╔══╗
SDI  ════║1 ║═══║1 ║═══ 0  ════ 1  ════
         ╚══╝   ╚══╝

        ←2us→←2us→
         一个 bit 周期 = 4us
         8 bits = 32us
```

#### 4.2.3 touchRead12() — 读取12位数据

```cpp
static uint16_t touchRead12() {
    uint16_t val = 0;                           // 初始化结果为0
    for (int i = 11; i >= 0; i--) {             // 从最高位读到最低位
        digitalWrite(T_CLK, HIGH);              // 时钟上升沿，XPT2046 输出数据
        delayMicroseconds(2);                   // 等待信号稳定
        val |= (digitalRead(T_SDO) << i);       // 读取并拼接到正确位置
        digitalWrite(T_CLK, LOW);               // 时钟下降沿
        delayMicroseconds(2);
    }
    return val;
}
```

**`val |= (digitalRead(T_SDO) << i)` 详解**：

分三步理解：
1. `digitalRead(T_SDO)` — 读取 MISO 引脚电平，返回 0 或 1
2. `<< i` — 左移 i 位，把这一位放到正确的位置
3. `val |= ...` — 用"按位或"合并到结果中

```
假设 MISO 依次返回: 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0

i=11: read=1 → 1<<11 = 100000000000 → val = 100000000000
i=10: read=0 → 0<<10 = 000000000000 → val = 100000000000 (不变)
i=9:  read=1 → 1<<9  = 000100000000 → val = 100100000000
i=8:  read=1 → 1<<8  = 000010000000 → val = 100110000000
i=7:  read=0 → 不变
i=6:  read=0 → 不变
i=5:  read=0 → 不变
i=4:  read=0 → 不变
i=3:  read=0 → 不变
i=2:  read=1 → 1<<2  = 000000000100 → val = 100110000100
i=1:  read=0 → 不变
i=0:  read=0 → 不变

最终 val = 0b100110000100 = 0x984 = 2436
```

#### 4.2.4 xptRead() — 完整的一次 SPI 通信

```cpp
uint16_t xptRead(uint8_t cmd) {
    digitalWrite(T_CS, LOW);       // ① 拉低 CS，选中 XPT2046
    touchWrite(cmd);               // ② 发送命令字节（告诉芯片读哪个通道）
    delayMicroseconds(100);        // ③ 等待 ADC 转换（约100微秒）
    uint16_t val = touchRead12();  // ④ 读取12位结果
    digitalWrite(T_CS, HIGH);      // ⑤ 拉高 CS，释放 XPT2046
    return val;
}
```

**完整时序图**：

```
        ┌───────────────────────────────────────────────────────┐
CS   ───┘                                                       └───
        ┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐                               ┌─┐
CLK  ───┘ └┘ └┘ └┘ └┘ └┘ └┘ └┘ └────────────────────────────────┘ └─
        ╔═══════════════════════╗    ┌───────────────────────────────
SDI  ═══║   命令 0xD0 (8 bits)  ║════┘
        ╚═══════════════════════╝
                                                     ┌───────────────
MISO ────────────────────────────────────────────────┤ XPT2046 返回数据
                                                     │ (12 bits)
                                                     └───────────────
        │← 发送命令 →│← 等待转换 →│←   读取数据    →│
        │   32us     │   100us    │     48us         │
```

#### 4.2.5 touchInit() — 初始化引脚

```cpp
void touchInit() {
    pinMode(T_CS, OUTPUT);
    digitalWrite(T_CS, HIGH);     // CS 默认高 = 不选中任何设备

    pinMode(T_CLK, OUTPUT);
    digitalWrite(T_CLK, LOW);     // SPI MODE0 要求时钟默认低

    pinMode(T_SDI, OUTPUT);
    digitalWrite(T_SDI, LOW);     // 数据线默认低

    pinMode(T_SDO, INPUT_PULLUP); // MISO 是输入，加内部上拉电阻
}
```

**为什么 SDO 要加 `INPUT_PULLUP`？**
- SDO 是 XPT2046 的输出引脚（从机 → 主机）
- 当 XPT2046 没被选中时，它的输出引脚是高阻态（悬空）
- 悬空引脚会拾取电磁干扰，读到随机的 0/1
- 内部上拉电阻把悬空状态拉到 HIGH（3.3V），避免误读

#### 4.2.6 isTouched() / touchRead() — 便捷接口

```cpp
bool isTouched() {
    return xptRead(0xB0) > TOUCH_Z_THRESHOLD;  // 压力超过阈值 = 有触摸
}

void touchRead(uint16_t &rawX, uint16_t &rawY, uint16_t &z) {
    rawX = xptRead(0xD0);  // 读 X 坐标
    rawY = xptRead(0x90);  // 读 Y 坐标
    z    = xptRead(0xB0);  // 读压力值
}
```

这些是封装好的便捷函数，让 `main.cpp` 不需要知道具体的命令字节。

---

## 5. display.h / display.cpp — 显示模块

### 5.1 TFT_eSPI 库

TFT_eSPI 是一个开源的 TFT 显示库，支持 ESP32/ESP8266/STM32 等平台。它的核心优势是**通过编译宏配置**，不需要改库源码。

**配置方式**（在 `platformio.ini` 中）：
```ini
build_flags =
    -D USER_SETUP_LOADED      # 跳过默认配置文件
    -D ILI9341_DRIVER          # 告诉库"我用的是 ILI9341 芯片"
    -D TFT_MOSI=16             # MOSI 接在 GPIO 16
    -D TFT_SCLK=18             # SCK 接在 GPIO 18
    ...
```

**`-D` 的作用**：等价于在代码开头写 `#define`。`-D TFT_MOSI=16` 等于 `#define TFT_MOSI 16`。

### 5.2 display.cpp 逐行解析

```cpp
#include "display.h"
#include "config.h"

static TFT_eSPI tft = TFT_eSPI();  // 创建 TFT 对象（static = 本文件私有）
```

**`static TFT_eSPI tft`**：这个 `tft` 对象只在 `display.cpp` 内可见。外部代码通过 `getTft()` 函数访问它。这叫**封装**——外部不能直接操作 `tft`，只能通过我们提供的接口。

```cpp
TFT_eSPI& getTft() {
    return tft;    // 返回引用，外部可以直接调用 tft 的方法
}
```

**返回引用 `TFT_eSPI&`**：调用者拿到的是 `tft` 本身，不是副本。这样调用者可以 `getTft().fillCircle(...)` 直接画图。

```cpp
void displayInit() {
    tft.init();              // 初始化硬件：配置 SPI3、发送 ILI9341 初始化序列
    tft.setRotation(0);      // 0=竖屏240×320, 1=横屏320×240
    tft.fillScreen(TFT_BLACK);  // 全屏填充黑色（清屏）
    tft.setTextFont(1);      // 使用 Font1（8×8 像素的 Adafruit GLCD 字体）
    tft.setTextColor(TFT_WHITE, TFT_BLACK);  // 前景白色，背景黑色
    tft.drawString("Touch Demo", 5, 5);       // 在 (5,5) 位置显示文字
}
```

**`setTextFont(1)` 字体编号**：
| 编号 | 名称 | 大小 | 说明 |
|:---:|:---|:---:|:---|
| 1 | GLCD | 8×8 | 默认小字体，适合显示数据 |
| 2 | Font2 | 12×16 | 中等字体，适合标题 |
| 4 | Font4 | 26px | 大字体，适合按钮文字 |

```cpp
void displayTouchInfo(uint16_t rawX, uint16_t rawY, uint16_t z, bool touched) {
    // 显示原始坐标值（黄色文字）
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("X:" + String(rawX) + "    ", 5, 20);  // "    " 清除旧数字
    tft.drawString("Y:" + String(rawY) + "    ", 5, 32);
    tft.drawString("Z:" + String(z) + "    ", 5, 44);

    // 显示触摸状态
    if (touched) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("TOUCHED!", 5, 56);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("no touch ", 5, 56);   // 末尾空格覆盖旧文字
    }
}
```

**为什么字符串末尾加空格？**`"TOUCHED!"` 有8个字符，`"no touch "` 也有8个字符。如果新字符串比旧字符串短，旧字符会残留在屏幕上。加空格确保覆盖。

---

## 6. main.cpp — 应用主程序

```cpp
#include <Arduino.h>
#include "config.h"      // 引脚定义
#include "touch.h"       // 触摸驱动接口
#include "display.h"     // 显示模块接口
```

模块化后，`main.cpp` 只需要包含接口头文件，不需要知道实现细节。

### 6.1 setup() — 初始化

```cpp
void setup() {
    Serial.begin(SERIAL_BAUD);  // 初始化 USB 串口
    delay(2000);                 // 等待 USB CDC 枚举完成

    displayInit();               // 初始化显示屏
    touchInit();                 // 初始化触摸引脚

    Serial.println("READY");
}
```

**为什么 `delay(2000)` ？**
ESP32-S3 使用原生 USB CDC 作为串口。上电后 USB 需要几秒钟完成枚举（和电脑握手）。如果不等，前面的 `Serial.println()` 输出会丢失。

### 6.2 loop() — 主循环

```cpp
void loop() {
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);                    // 读取触摸值

    bool touched = (z > TOUCH_Z_THRESHOLD);      // 判断是否触摸
    displayTouchInfo(rawX, rawY, z, touched);     // 更新屏幕

    if (touched) {
        Serial.printf("rawX=%d rawY=%d z=%d\n", rawX, rawY, z);
        getTft().fillCircle(120, 160, 5, TFT_GREEN);  // 画绿点
    }

    delay(100);  // 每100ms 循环一次
}
```

**`Serial.printf()` 格式化输出**：
- `%d` = 十进制整数
- `%x` = 十六进制
- `%f` = 浮点数
- `\n` = 换行

**`getTft().fillCircle(120, 160, 5, TFT_GREEN)`**：
- `getTft()` 获取 TFT 对象
- `.fillCircle(x, y, r, color)` 画实心圆
- 在屏幕中心 (120, 160) 画半径5像素的绿色圆

---

## 7. platformio.ini — 构建配置

```ini
[env:esp32s3]                            # 环境名称
platform = espressif32                   # 芯片平台：ESP32 系列
board = esp32-s3-devkitc-1               # 开发板型号
framework = arduino                      # 使用 Arduino 框架
monitor_speed = 115200                   # 串口监视器波特率

lib_deps =                               # 依赖库（PlatformIO 自动下载）
    bodmer/TFT_eSPI@^2.5.43             # TFT 显示库
    paulstoffregen/XPT2046_Touchscreen   # 触摸库（本项目未使用，但保留备用）

build_flags =                            # 编译宏定义
    -D USER_SETUP_LOADED                 # 跳过 TFT_eSPI 默认配置
    -D ILI9341_DRIVER                    # 显示驱动芯片
    -D TFT_MOSI=16                       # SPI 引脚...
    -D TFT_SCLK=18
    -D TFT_CS=4
    -D TFT_DC=5
    -D TFT_RST=6
    -D TFT_MISO=15
    -D LOAD_GLCD                         # 加载 Font1 (8×8)
    -D LOAD_FONT2                        # 加载 Font2 (12×16)
    -D LOAD_FONT4                        # 加载 Font4 (26px)
    -D SMOOTH_FONT                       # 启用平滑字体渲染
    -D SPI_FREQUENCY=40000000            # 显示 SPI 频率 40MHz
    -D SPI_TOUCH_FREQUENCY=2500000       # 触摸 SPI 频率 2.5MHz
    -D USE_HSPI_PORT                     # 使用 SPI3 驱动显示屏
    -D ARDUINO_USB_CDC_ON_BOOT=1         # 启用 USB 串口输出
```

**`lib_deps` 的作用**：告诉 PlatformIO 从库注册表下载指定库。`@^2.5.43` 表示版本 2.5.43 及以上（不包含3.x）。

**`USE_HSPI_PORT` 的含义**：
- ESP32-S3 有 SPI2 和 SPI3 两个通用 SPI 外设
- `USE_HSPI_PORT` 让 TFT_eSPI 使用 SPI3
- 如果不指定，TFT_eSPI 默认用 SPI2，可能和 Flash 冲突

---

## 8. 踩坑全记录

### 坑1：GPIO 11/12/13 黑屏

**现象**：屏幕全黑，芯片可能无法启动
**原因**：GPIO 11(SPIWP)、12(SPIHD)、13(SPICLK) 是 ESP32-S3 的 SPI Flash 接口
**教训**：先查数据手册的引脚功能表，再选 GPIO

### 坑2：ILI9341 模块 T_SDI 不连通

**现象**：触摸读到全0或固定值4095
**原因**：ILI9341 模块的 SDI(MOSI) 和 T_SDI 在 PCB 内部**没有连通**
**排查过程**：
1. 万用表蜂鸣档测 T_SDI 和 SDI → 不响（不导通）
2. 从 T_SDI 单独拉一根线到 ESP32 GPIO 21 → 触摸有数据了
**教训**：模块上的同类引脚不一定内部连通，需要万用表确认

### 坑3：显示和触摸共享 SPI 总线

**现象**：`getTouchRaw()` 返回全0
**原因**：TFT_eSPI 用 SPI3 驱动显示，手动 SPI 读触摸用的是 SPI2，两个 SPI 外设控制不同的 GPIO，数据读不到
**解决方案**：触摸改用 bit-bang（手动翻转 GPIO），完全独立于硬件 SPI

### 坑4：USE_HSPI_PORT 去不掉

**现象**：去掉 `USE_HSPI_PORT` 后板子崩溃（Guru Meditation Error）
**原因**：ESP32-S3 的默认 SPI 引脚和 Flash 冲突，必须指定用 SPI3
**教训**：ESP32-S3 的 SPI 配置和 ESP32 不同，不能照搬

### 坑5：字体不显示

**现象**：`fillScreen` 能显示颜色，但 `drawString` 什么都不画
**原因**：`USER_SETUP_LOADED` 跳过了 `User_Setup.h`，里面的 `LOAD_GLCD`、`LOAD_FONT2` 没有被定义，字体数据没编译进去
**解决方案**：在 `build_flags` 里加 `-D LOAD_GLCD -D LOAD_FONT2`

### 坑6：ESP32-S3 串口无输出

**现象**：`Serial.println()` 没有输出
**原因**：ESP32-S3 的 USB 原生 CDC 需要额外配置
**解决方案**：加 `-D ARDUINO_USB_CDC_ON_BOOT=1`

### 坑7：校准坐标不准

**现象**：触摸位置和显示位置偏差很大
**原因**：XPT2046 的原始值需要校准映射到屏幕坐标，且触摸轴可能和显示轴交换
**未解决**：需要四点校准算法，后续实现

---

## 9. 扩展学习

### 9.1 下一步改进方向

1. **触摸校准**：四点校准算法，自动计算 raw→screen 映射
2. **手写绘图**：触摸画画，支持清屏和颜色选择
3. **GUI 组件**：按钮、滑块、菜单
4. **中断驱动触摸**：用 T_IRQ 引脚触发中断，避免轮询
5. **双缓冲**：减少屏幕闪烁

### 9.2 推荐学习资源

- [ILI9341 数据手册](https://www.displayfuture.com/Display/datasheet/controller/ILI9341.pdf) — 命令列表在 Section 8
- [XPT2046 数据手册](https://www.waveshare.com/w/upload/8/82/Xpt2046.pdf) — SPI 协议在 Section 7
- [ESP32-S3 技术参考手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) — GPIO 和 SPI 章节
- [TFT_eSPI GitHub](https://github.com/Bodmer/TFT_eSPI) — 配置指南和示例
- [PlatformIO 文档](https://docs.platformio.org/) — 构建系统配置

### 9.3 关键概念速查

| 概念 | 解释 |
|:---|:---|
| SPI | Serial Peripheral Interface，同步串行协议，4线（CLK/MOSI/MISO/CS） |
| Bit-bang | 不用硬件 SPI 外设，手动翻转 GPIO 实现协议 |
| CS/SS | Chip Select，低电平选中设备，实现多设备共享总线 |
| ADC | Analog-to-Digital Converter，模拟转数字 |
| RGB565 | 16-bit 颜色格式：5位红+6位绿+5位蓝 |
| GPIO | General Purpose Input/Output，通用输入输出引脚 |
| Pull-up | 上拉电阻，把悬空引脚拉到 HIGH |
| Strapping Pin | 启动模式引脚，上电时的状态决定芯片行为 |
| USB CDC | USB Communications Device Class，USB 虚拟串口 |
