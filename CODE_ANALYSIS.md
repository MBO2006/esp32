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
8. [main.cpp — 应用主程序](#8-maincpp--应用主程序)
9. [platformio.ini — 构建配置](#9-platformioini--构建配置)
10. [踩坑全记录](#10-踩坑全记录)
11. [扩展学习](#11-扩展学习)

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
esp32/
├── platformio.ini          ← 构建配置（引脚、库、编译选项）
├── CODE_ANALYSIS.md        ← 本文档
├── CHANGELOG.md            ← 版本变更记录
├── README.md               ← 项目说明与接线教程
├── tools/
│   ├── font_converter.py   ← 字体转换脚本（Python，命令行版）
│   └── font_converter_gui.py ← 字体转换 GUI 工具
└── src/
    ├── config.h            ← ① 引脚定义、全局常量
    ├── touch.h             ← ② 触摸驱动接口声明
    ├── touch.cpp           ← ② 触摸驱动实现（bit-bang SPI）
    ├── chinese_font.h      ← ③ 中文字库（自动生成的位图数据）
    ├── display.h           ← ④ 显示模块接口 + Button/Page 定义
    ├── display.cpp         ← ④ 显示模块实现（页面绘制、按钮）
    ├── loop.h              ← ⑤ 主循环接口
    ├── loop.cpp            ← ⑤ 主循环实现（触摸检测、页面切换）
    └── main.cpp            ← ⑥ 应用入口（setup 初始化）
```

### 模块依赖关系

```
main.cpp
  ├── config.h        (引脚常量)
  ├── touch.h/cpp     (初始化触摸)
  └── display.h/cpp   (初始化屏幕、画首页)

loop.cpp
  ├── config.h        (TOUCH_Z_THRESHOLD)
  ├── touch.h/cpp     (读触摸坐标)
  └── display.h/cpp   (页面切换、按钮检测)

display.cpp
  ├── config.h        (引脚常量)
  ├── touch.h         (仅 include，未直接使用)
  ├── chinese_font.h  (中文字模位图)
  └── TFT_eSPI 库     (底层显示驱动)

touch.cpp
  ├── config.h        (引脚常量)
  └── display.h       (仅 include，未直接使用)
```

**设计原则**：每个模块只做一件事。改引脚只动 `config.h`，改触摸逻辑只动 `touch.cpp`，改界面只动 `display.cpp`，改页面路由只动 `loop.cpp`。

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
#include "display.h"  // 引入显示模块（当前未直接使用）
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

这些是封装好的便捷函数，让 `loop.cpp` 不需要知道具体的命令字节。

---

## 5. chinese_font.h — 中文字库

### 5.1 为什么需要自定义字库？

TFT_eSPI 库内置了英文字体（GLCD、Font2、Font4），但**不包含中文字库**。中文字符有几千个，全部内置会占用巨大的 Flash 空间。解决方案是：**只包含项目实际用到的汉字**，用 Python 工具从系统字体中提取。

### 5.2 字库结构

```cpp
// 自动生成的中文字库 — 请勿手动修改
// 字体: msyh.ttc, 字号: 24px
// 字符数: 28

#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

#define CN_FONT_SIZE 24     // 字号（像素）
#define CN_CHAR_COUNT 28    // 包含的字符数
```

每个汉字被转换为一个 `uint16_t` 数组，存在 Flash（PROGMEM）中：

```cpp
// '佳' (U+4F73) 32x30
static const uint16_t cn_char_000[] PROGMEM = {
    0x0000, 0x0000, ...  // 32×30 = 960 个像素的 RGB565 颜色值
};
```

**PROGMEM 关键字**：告诉编译器把数据放在 Flash 而不是 RAM 中。ESP32-S3 有 8MB Flash 但只有 512KB RAM，字库数据必须放 Flash。

### 5.3 字模查找表

字库文件末尾有查找表，把 Unicode 码点映射到位图数组：

```cpp
// 查找表：输入汉字 → 输出位图数组指针和尺寸
struct CnCharInfo {
    const uint16_t *data;   // 位图数据指针
    uint16_t width;          // 字符宽度（像素）
    uint16_t height;         // 字符高度（像素）
};
```

### 5.4 cnDrawString() — 绘制中文字符串

显示模块通过 `cnDrawString()` 函数渲染中文文本：

```cpp
cnDrawString(&tft, 10, 10, "这是菜单");
```

**工作流程**：
1. 遍历字符串中的每个字符
2. 在查找表中找到对应的位图数据
3. 用 TFT_eSPI 的 `pushImage()` 逐行绘制位图到屏幕

### 5.5 字体转换工具

项目提供了 Python 工具来生成字库文件：

| 工具 | 说明 |
|:---|:---|
| `tools/font_converter.py` | 命令行版：`python font_converter.py --text "你好世界" --output font.h` |
| `tools/font_converter_gui.py` | GUI 版：带图形界面，可视化选择字体和字号 |

**使用场景**：需要显示新的中文文字时，用工具把新文字追加到字库中，重新编译即可。

---

## 6. display.h / display.cpp — 显示模块

### 6.1 TFT_eSPI 库

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

### 6.2 display.h — 接口声明

```cpp
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// ===== 按钮结构体 =====
struct Button {
    uint16_t x, y, w, h;   // 按钮位置和大小
    const char *label;      // 按钮标签（支持中文）
};

// ===== 页面枚举 =====
enum Page {
    PAGE_HOME,       // 首页（菜单选择）
    PAGE_FUNCTION,   // 功能页
    PAGE1,           // 页面1
    PAGE2,           // 页面2
    PAGE_COUNT       // 页面总数（哨兵值）
};

// ===== 按钮声明（定义在 loop.cpp）=====
extern Button page1;
extern Button page2;
extern Button backBtn;
extern Button functionBtn;

// ===== 显示函数 =====
TFT_eSPI &getTft();              // 获取 TFT 对象
void displayInit();              // 初始化显示屏
void displayTouchInfo(bool touched);  // 显示触摸状态
Page getCurrentPage();           // 获取当前页面

// ===== 页面绘制 =====
void drawPageHome();             // 首页
void drawPageFunction();         // 功能页
void drawpage1();                // 页面1
void drawpage2();                // 页面2

// ===== 页面切换 =====
void switchToHome();             // 切换到首页
void switchToFunction();         // 切换到功能页
void gotopage1();                // 切换到页面1
void gotopage2();                // 切换到页面2

// ===== 按钮操作 =====
void drawButton(const Button &btn);
bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn);
```

**`struct Button`**：
```cpp
struct Button {
    uint16_t x, y, w, h;  // 按钮左上角坐标 + 宽高
    const char *label;     // 按钮上显示的文字（支持中文UTF-8）
};
```
结构体把相关的数据打包在一起。一个 `Button` 变量包含了绘制和检测按钮所需的全部信息。

**`enum Page` 页面枚举**：
```cpp
enum Page {
    PAGE_HOME,      // = 0，首页
    PAGE_FUNCTION,  // = 1，功能页
    PAGE1,          // = 2，页面1
    PAGE2,          // = 3，页面2
    PAGE_COUNT      // = 4，页面总数（不作为实际页面使用）
};
```
枚举用有意义的名字代替魔法数字，代码更易读。`PAGE_COUNT` 是常见技巧——它自动等于前面成员的数量，可用于数组大小或范围检查。

**`extern` 关键字**：
```cpp
extern Button page1;
```
告诉编译器："这个变量在别的 `.cpp` 文件里定义了，我这里只是声明，链接时再去找"。按钮的实际定义在 `loop.cpp` 中。

### 6.3 display.cpp 逐行解析

#### 6.3.1 静态变量

```cpp
#include "display.h"
#include "config.h"
#include "touch.h"
#include "chinese_font.h"     // 引入中文字库

static TFT_eSPI tft = TFT_eSPI();    // 创建 TFT 对象（static = 本文件私有）
static Page currentPage = PAGE_HOME;  // 当前页面状态（默认首页）
```

**`static TFT_eSPI tft`**：这个 `tft` 对象只在 `display.cpp` 内可见。外部代码通过 `getTft()` 函数访问它。这叫**封装**——外部不能直接操作 `tft`，只能通过我们提供的接口。

**`static Page currentPage`**：记录当前显示的页面。所有页面切换函数都会更新它。

#### 6.3.2 accessor 函数

```cpp
TFT_eSPI &getTft() {
    return tft;            // 返回引用，外部可以直接调用 tft 的方法
}

Page getCurrentPage() {
    return currentPage;    // 返回当前页面枚举值
}
```

**返回引用 `TFT_eSPI&`**：调用者拿到的是 `tft` 本身，不是副本。这样调用者可以 `getTft().fillCircle(...)` 直接画图。

#### 6.3.3 displayInit() — 初始化显示屏

```cpp
void displayInit() {
    tft.init();              // 初始化硬件：配置 SPI3、发送 ILI9341 初始化序列
}
```

#### 6.3.4 displayTouchInfo() — 显示触摸状态

```cpp
void displayTouchInfo(bool touched)
{
    if (touched) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("TOUCHED!", 140, 20);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("no touch ", 140, 20);
    }
}
```

**为什么字符串末尾加空格？** `"TOUCHED!"` 有8个字符，`"no touch "` 也有8个字符。如果新字符串比旧字符串短，旧字符会残留在屏幕上。加空格确保覆盖。

#### 6.3.5 页面绘制函数

每个页面函数负责：清屏 → 画文字 → 画按钮。

**首页 `drawPageHome()`**：
```cpp
void drawPageHome() {
    tft.fillScreen(TFT_BLACK);                    // 清屏
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);                           // Font2 (12×16)
    cnDrawString(&tft, 10, 10, "这是菜单");        // 中文标题
    cnDrawString(&tft, 10, 40, "你可以选择页面并点击"); // 中文提示
    drawButton(page1);                            // 画"页面1"按钮
    drawButton(page2);                            // 画"页面2"按钮
}
```

**页面1 `drawpage1()`**：
```cpp
void drawpage1() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("PAGE1", 10, 10);
    drawButton(backBtn);          // "返回"按钮
    drawButton(functionBtn);      // "功能"按钮
}
```

**功能页 `drawPageFunction()`**：
```cpp
void drawPageFunction() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("FUNCTION PAGE", 10, 10);
    cnDrawString(&tft, 10, 40, "佳佳快学习");     // 中文内容
    cnDrawString(&tft, 10, 80, "琼琼别学了");     // 中文内容
    drawButton(backBtn);                          // "返回"按钮
}
```

#### 6.3.6 页面切换函数

切换函数的模式统一：**更新状态 + 重绘页面**。

```cpp
void switchToHome() {
    currentPage = PAGE_HOME;   // 更新状态变量
    drawPageHome();            // 重绘目标页面
}

void switchToFunction() {
    currentPage = PAGE_FUNCTION;
    drawPageFunction();
}

void gotopage1() {
    currentPage = PAGE1;
    drawpage1();
}

void gotopage2() {
    currentPage = PAGE2;
    drawpage2();
}
```

#### 6.3.7 drawButton() — 绘制按钮

```cpp
void drawButton(const Button &btn) {
    tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_BLACK);            // 边框
    tft.fillRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, TFT_BLACK); // 填充（内缩1像素）
    tft.setTextColor(TFT_BLACK, TFT_BLACK);                          // 文字颜色
    tft.setTextFont(2);                                               // 12×16 字体
    cnDrawString(&tft, btn.x + 10, btn.y + 8, btn.label);           // 绘制中文标签
}
```

**`const Button &btn`**：
- `const`：函数内不修改按钮数据
- `&`：引用传递，不复制整个结构体，更高效

#### 6.3.8 isInButton() — 触摸命中检测

```cpp
bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn) {
    return (rawX >= btn.x && rawX <= (btn.x + btn.w) &&
            rawY >= btn.y && rawY <= (btn.y + btn.h));
}
```

**矩形碰撞检测**：判断点 (rawX, rawY) 是否在矩形 [x, x+w] × [y, y+h] 内。

```
        btn.x           btn.x + btn.w
          │                   │
          ▼                   ▼
    ┌─────┬─────────────────┬─────┐
    │     │                 │     │  ← btn.y
    │     │    按钮区域      │     │
    │     │                 │     │
    │     │  (rawX,rawY) ●  │     │
    │     │                 │     │
    └─────┴─────────────────┴─────┘  ← btn.y + btn.h

    触摸点在区域内 → 返回 true
```

---

## 7. loop.h / loop.cpp — 主循环与页面逻辑

### 7.1 为什么单独一个模块？

在早期版本中，`loop()` 函数直接写在 `main.cpp` 里。随着页面和按钮数量增加，逻辑越来越复杂，所以拆分为独立模块。这符合**单一职责原则**：
- `main.cpp` 只负责初始化
- `loop.cpp` 只负责运行时逻辑（触摸检测、页面路由）

### 7.2 loop.h — 接口

```cpp
#pragma once

void loop();  // 声明自定义的 loop 函数
```

**注意**：Arduino 框架默认提供 `loop()` 函数，但这个项目**用自己的 `loop()` 替代了它**（在 `main.cpp` 的 `setup()` 末尾手动调用）。

### 7.3 loop.cpp — 完整实现

#### 7.3.1 头部与按钮定义

```cpp
#include "loop.h"
#include "display.h"
#include "touch.h"
#include "config.h"

// 按钮定义（变量名全小写）
Button page1 = {20, 278, 100, 40, "页面1"};
Button page2 = {140, 278, 100, 40, "页面2"};
Button backBtn = {20, 278, 100, 40, "返回"};
Button functionBtn = {140, 278, 100, 40, "功能"};
```

**按钮坐标说明**（240×320 竖屏）：
```
屏幕 240×320
┌──────────────────────┐
│                      │ y=0
│   这是菜单            │
│   你可以选择页面并点击 │
│                      │
│                      │
│                      │
│                      │
│                      │
│  ┌────────┐ ┌────────┐│ ← y=278
│  │  页面1  │ │  页面2  ││ ← h=40
│  └────────┘ └────────┘│ ← y=318
└──────────────────────┘
   x=20       x=140
   w=100      w=100
```

`backBtn` 和 `functionBtn` 在不同的页面中使用，位置可以相同也可以不同。

#### 7.3.2 loop() — 主循环

```cpp
void loop() {
    // ① 读取触摸
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);

    // ② 判断是否触摸
    bool touched = (z > TOUCH_Z_THRESHOLD);
    displayTouchInfo(touched);

    // ③ 如果有触摸
    if (touched) {
        // 校准映射：原始值 → 屏幕像素
        uint16_t screenX = map(rawX, 220, 1780, 0, 239);
        uint16_t screenY = map(rawY, 200, 1830, 0, 319);

        // 串口调试输出
        Serial.printf("rawX=%d rawY=%d z=%d  →  screenX=%d screenY=%d\n",
                      rawX, rawY, z, screenX, screenY);
        Serial.flush();

        // ④ 获取当前页面，根据页面分发按钮事件
        Page currentPage = getCurrentPage();

        if (currentPage == PAGE_HOME) {
            if (isInButton(screenX, screenY, page1)) {
                gotopage1();           // 首页 → 页面1
            } else if (isInButton(screenX, screenY, page2)) {
                gotopage2();           // 首页 → 页面2
            }
        }
        if (currentPage == PAGE1) {
            if (isInButton(screenX, screenY, backBtn)) {
                switchToHome();        // 页面1 → 首页
            } else if (isInButton(screenX, screenY, functionBtn)) {
                switchToFunction();    // 页面1 → 功能页
            }
        }
        if (currentPage == PAGE2) {
            if (isInButton(screenX, screenY, backBtn)) {
                switchToHome();        // 页面2 → 首页
            }
        }
        if (currentPage == PAGE_FUNCTION) {
            if (isInButton(screenX, screenY, backBtn)) {
                gotopage1();           // 功能页 → 页面1
            }
        }
    }
    delay(100);  // 每100ms循环一次
}
```

### 7.4 页面路由图

```
                  ┌─────────────┐
                  │  PAGE_HOME  │
                  │   "这是菜单"  │
                  └──┬──────┬──┘
            点"页面1" │      │ 点"页面2"
                     ▼      ▼
          ┌──────────┐    ┌──────────┐
          │  PAGE1   │    │  PAGE2   │
          │  "PAGE1" │    │  "PAGE2" │
          └──┬───┬───┘    └────┬─────┘
             │   │             │
    点"功能"  │   │ 点"返回"     │ 点"返回"
             ▼   │             │
     ┌──────────┐│             │
     │FUNCTION  ││             │
     │  PAGE    ││             │
     └────┬─────┘│             │
          │      │             │
  点"返回"│      │             │
          ▼      ▼             ▼
         回到 PAGE1     回到 PAGE_HOME
```

### 7.5 触摸校准详解

**问题**：XPT2046 返回的原始值范围是 0~4095，但实际触摸范围远小于此。
**解决**：通过四角校准获取实际范围，用 `map()` 线性映射。

```
原始值空间（0~4095）              屏幕空间（240×320）
┌─────────────────────┐          ┌─────────────────────┐
│                     │          │ (0,0)       (239,0) │
│   220         1780  │          │                     │
│    ┌───────────┐    │          │                     │
│ 200│  实际触摸  │1830│    →     │                     │
│    │   区域     │    │          │                     │
│    └───────────┘    │          │                     │
│                     │          │ (0,319)    (239,319) │
└─────────────────────┘          └─────────────────────┘

map(value, fromLow, fromHigh, toLow, toHigh)
map(rawX, 220,    1780,      0,     239  )  → screenX
map(rawY, 200,    1830,      0,     319  )  → screenY
```

**校准数据**（实测四角平均值）：

| 屏幕位置 | rawX | rawY |
|----------|------|------|
| 左上 | ~255 | ~215 |
| 右上 | ~1750 | ~215 |
| 右下 | ~1765 | ~1820 |
| 左下 | ~230 | ~1785 |

取安全范围：X → [220, 1780]，Y → [200, 1830]

### 7.6 Arduino `map()` 函数

```cpp
long map(long value, long fromLow, long fromHigh, long toLow, long toHigh)
```

本质是线性插值公式：
```
result = (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow
```

例：`map(1000, 220, 1780, 0, 239)`
```
= (1000 - 220) * (239 - 0) / (1780 - 220) + 0
= 780 * 239 / 1560
= 119
```
触摸原始值 1000 对应屏幕 X 坐标 119（屏幕中央）。

---

## 8. main.cpp — 应用主程序

```cpp
/**
 * main.cpp — 应用主程序
 *
 * 功能：读取 XPT2046 触摸值并在 ILI9341 屏幕上显示
 *
 * 模块依赖：
 *   config.h    — 引脚定义
 *   touch.h/cpp — XPT2046 触摸驱动
 *   display.h/cpp — 显示辅助函数
 */
#include <Arduino.h>
#include "config.h"
#include "touch.h"
#include "display.h"

void setup()
{
    Serial.begin(SERIAL_BAUD);   // 初始化 USB 串口（115200 bps）
    delay(2000);                  // 等待 USB CDC 枚举完成
    Serial.println("=== Touch Demo ===");
    Serial.flush();               // 等待数据发送完毕

    displayInit();                // 初始化 ILI9341 显示屏
    drawPageHome();               // 画首页（含"页面1""页面2"按钮）

    touchInit();                  // 初始化 XPT2046 触摸引脚
    loop();                      // 进入主循环（检测触摸并处理页面切换）

    Serial.println("READY");     // 实际上 loop() 不会返回，这行不会执行
    Serial.flush();
}
```

**为什么 `delay(2000)` ？**
ESP32-S3 使用原生 USB CDC 作为串口。上电后 USB 需要几秒钟完成枚举（和电脑握手）。如果不等，前面的 `Serial.println()` 输出会丢失。

**`Serial.flush()`**：等待串口发送缓冲区清空。确保调试信息在崩溃前输出。

**注意**：`main.cpp` 极其精简——它只做初始化，然后调用 `loop()` 进入循环。所有运行时逻辑都在 `loop.cpp` 中。

### 完整执行流程

```
上电
 │
 ├─ setup()
 │   ├─ Serial.begin(115200)     初始化串口
 │   ├─ delay(2000)              等待 USB 就绪
 │   ├─ displayInit()            初始化屏幕
 │   ├─ drawPageHome()           画首页（标题 + 按钮）
 │   ├─ touchInit()              初始化触摸引脚
 │   └─ loop()                   进入主循环 ← 不会返回
 │       └─ 详见 loop.cpp 解析
```

---

## 9. platformio.ini — 构建配置

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
    -D TOUCH_CS=7                        # 触摸片选（未使用，保留）
    -D TOUCH_IRQ=17                      # 触摸中断（未使用，保留）
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

## 10. 踩坑全记录

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

### 坑7：触摸坐标校准（已解决）

**现象**：触摸位置和显示位置偏差很大
**原因**：XPT2046 的原始值（0~4095）不等于屏幕像素（240×320），需要校准映射
**解决方案**：
1. 串口打印四角原始值
2. 取平均值确定实际范围（X: 220~1780, Y: 200~1830）
3. 用 `map()` 做线性映射

### 坑8：PlatformIO 迁移后编译失败

**现象**：将 PlatformIO 从 C 盘迁移到 D 盘后，VS Code 扩展报错
**原因**：`penv`（Python 虚拟环境）里的路径还是指向旧的 C 盘位置
**解决方案**：
1. 设置环境变量 `PLATFORMIO_CORE_DIR=D:\PlatformIO`
2. 删除旧 `penv`，用 `pip install platformio` 重新安装
3. 在 `.vscode/settings.json` 中配置 `"platformio.customPATH": "E:\\Python\\Scripts"`

---

## 11. 扩展学习

### 11.1 已实现的功能

- [x] 触摸读取与串口输出
- [x] 屏幕显示触摸状态
- [x] 触摸坐标校准（四角校准 + map 映射）
- [x] 中文字库（msyh 24px，28个汉字）
- [x] 多页面系统（首页 → 页面1/页面2 → 功能页）
- [x] 按钮导航（页面切换 + 返回）
- [x] 代码模块化（config/touch/display/loop/main）

### 11.2 下一步改进方向

1. **按钮按下反馈**：按下时变色，松开恢复
2. **防抖处理**：加时间间隔，避免一次触摸触发多次
3. **过渡动画**：页面切换时的淡入淡出效果
4. **更多页面内容**：每个子页面添加实际功能（传感器数据、设置等）
5. **中断驱动触摸**：用 T_IRQ 引脚触发中断，避免轮询
6. **手环 UI**：圆形/弧形界面适配小屏幕手环场景

### 11.3 推荐学习资源

- [ILI9341 数据手册](https://www.displayfuture.com/Display/datasheet/controller/ILI9341.pdf) — 命令列表在 Section 8
- [XPT2046 数据手册](https://www.waveshare.com/w/upload/8/82/Xpt2046.pdf) — SPI 协议在 Section 7
- [ESP32-S3 技术参考手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) — GPIO 和 SPI 章节
- [TFT_eSPI GitHub](https://github.com/Bodmer/TFT_eSPI) — 配置指南和示例
- [PlatformIO 文档](https://docs.platformio.org/) — 构建系统配置

### 11.4 关键概念速查

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
| PROGMEM | 把数据放在 Flash 而非 RAM 中，节省内存 |
| map() | Arduino 线性映射函数，将一个范围的值映射到另一个范围 |
| struct | C/C++ 结构体，将多个相关变量打包成一个类型 |
| enum | 枚举类型，用有意义的名字代替魔法数字 |
| extern | 声明变量/函数在其他文件中定义，链接时解析 |
| const 引用 | `const T&` — 传引用不复制，且保证函数内不修改数据 |
| UTF-8 | 可变长度字符编码，英文1字节，中文3字节 |
