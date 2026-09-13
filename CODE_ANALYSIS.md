# 代码解析

本文档解析 ESP32-S3 + ILI9341 + XPT2046 触摸屏项目的每一行代码，适合嵌入式开发学习。

---

## 1. 项目架构

```
src/
├── config.h          ← 引脚定义、常量
├── touch.h / touch.cpp  ← XPT2046 触摸驱动（bit-bang SPI）
├── display.h / display.cpp ← 显示辅助函数
└── main.cpp          ← 应用逻辑（setup + loop）
```

**为什么要模块化？**
- `main.cpp` 只关心"做什么"（应用逻辑）
- `touch.cpp` 关心"怎么读触摸"（底层驱动）
- `display.cpp` 关心"怎么画界面"（UI 辅助）
- 修改引脚只需改 `config.h`，不影响其他代码

---

## 2. config.h — 引脚与常量

```cpp
#pragma once

// ===== 显示 SPI（TFT_eSPI 管理，通过 platformio.ini 配置）=====
// MOSI=16, SCK=18, MISO=15, CS=4, DC=5, RST=6

// ===== 触摸 SPI（手动 bit-bang，不经过 SPI 库）=====
#define T_CS   7    // 触摸芯片片选
#define T_CLK  14   // 触摸 SPI 时钟
#define T_SDI  21   // 触摸数据输入（ESP32 → XPT2046）
#define T_SDO  47   // 触摸数据输出（XPT2046 → ESP32）

// ===== 触摸参数 =====
#define TOUCH_Z_THRESHOLD  50   // 压力阈值，低于此值视为未触摸
```

### 知识点：`#pragma once`
防止头文件被重复包含。如果没有它，`main.cpp` 和 `touch.cpp` 都 `#include "config.h"` 时会报"重复定义"错误。

---

## 3. touch.h / touch.cpp — XPT2046 触摸驱动

### 3.1 什么是 XPT2046？

XPT2046 是一颗电阻式触摸屏控制芯片，通过 SPI 协议通信：
- **输入**：触摸屏的 X+/X-/Y+/Y- 四线模拟信号
- **输出**：12-bit 数字坐标（0~4095）
- **通信**：SPI（时钟、数据输入、数据输出、片选）

### 3.2 为什么用 bit-bang 而不用 SPI 库？

ESP32-S3 有3个 SPI 硬件外设：
- **SPI0/SPI1**：连接 Flash/PSRAM，**绝对不能碰**
- **SPI2 (FSPI)**：通用 SPI
- **SPI3 (HSPI)**：通用 SPI，本项目用于驱动显示屏（TFT_eSPI）

如果触摸也用硬件 SPI，需要和显示屏共享总线，容易冲突。**bit-bang（手动翻转 GPIO）** 虽然慢一些，但完全独立，不会干扰显示屏。

### 3.3 SPI 通信协议

```
        ┌───┐   ┌───┐   ┌───┐   ┌───┐
CLK  ───┘   └───┘   └───┘   └───┘   └───
        ┌─────────────────────────────────
CS   ───┘
        ╔═══╗   ╔═══╗   ╔═══╗   ╔═══╗
MOSI ═══║D7 ║═══║D6 ║═══║D5 ║═══║D4 ║═══  (主机发命令)
        ╚═══╝   ╚═══╝   ╚═══╝   ╚═══╝
                    ┌───────────┐
MISO ───────────────┤ XPT2046   ├───────  (从机回数据)
                    │ 返回数据   │
                    └───────────┘
```

**SPI MODE0**：数据在时钟**上升沿**采样，在**下降沿**切换。

### 3.4 代码逐行解析

```cpp
// 发送 8-bit 命令到 XPT2046
void touchWrite(uint8_t data) {
    for (int i = 7; i >= 0; i--) {          // 从最高位(MSB)开始发送
        digitalWrite(T_SDI, (data >> i) & 1); // 把第 i 位放到数据线上
        digitalWrite(T_CLK, HIGH);             // 时钟拉高（上升沿）
        delayMicroseconds(2);                  // 保持2微秒
        digitalWrite(T_CLK, LOW);              // 时钟拉低（下降沿）
        delayMicroseconds(2);                  // 保持2微秒
    }
}
```

**`data >> i` 位移操作解析：**
```
data = 0xD0 = 11010000 (二进制)

i=7: data >> 7 = 1  → T_SDI = 1
i=6: data >> 6 = 3  → 3 & 1 = 1 → T_SDI = 1
i=5: data >> 5 = 6  → 6 & 1 = 0 → T_SDI = 0
i=4: data >> 4 = 13 → 13 & 1 = 1 → T_SDI = 1
...以此类推
```

```cpp
// 从 XPT2046 读取 12-bit 数据
uint16_t touchRead12() {
    uint16_t val = 0;
    for (int i = 11; i >= 0; i--) {       // 读12位
        digitalWrite(T_CLK, HIGH);          // 时钟上升沿
        delayMicroseconds(2);
        val |= (digitalRead(T_SDO) << i);  // 读取数据线，左移到正确位置
        digitalWrite(T_CLK, LOW);           // 时钟下降沿
        delayMicroseconds(2);
    }
    return val;
}
```

**`val |= (digitalRead(T_SDO) << i)` 解析：**
```
假设 MISO 依次返回: 1,0,1,1,0,0,0,0,0,1,0,0

i=11: read=1 → val = 000000000000 | 100000000000 = 100000000000
i=10: read=0 → val = 100000000000 | 000000000000 = 100000000000
i=9:  read=1 → val = 100000000000 | 000100000000 = 100100000000
...
最终 val = 0xB04 = 2820
```

```cpp
// 完整的一次读取：发命令 + 读数据
uint16_t xptRead(uint8_t cmd) {
    digitalWrite(T_CS, LOW);       // 选中 XPT2046（CS 拉低）
    touchWrite(cmd);               // 发送命令字节
    delayMicroseconds(100);        // 等待 ADC 转换（~100us）
    uint16_t val = touchRead12();  // 读取 12-bit 结果
    digitalWrite(T_CS, HIGH);      // 释放 XPT2046（CS 拉高）
    return val;
}
```

### 3.5 XPT2046 命令字节

| 命令 | 二进制 | 功能 |
|:---:|:---|:---|
| 0xD0 | 11010000 | 读取 X 坐标 |
| 0x90 | 10010000 | 读取 Y 坐标 |
| 0xB0 | 10110000 | 读取 Z1 压力 |

命令格式：`S A2 A1 A0 MODE SER/DFR PD1 PD0`
- **S=1**：启动位
- **A2-A0**：通道选择（001=X, 101=Y, 011=Z1）
- **MODE=1**：12-bit 模式（=0 则 8-bit）
- **SER/DFR=0**：差分模式（精度更高）
- **PD1 PD0=00**：省电模式

---

## 4. display.h / display.cpp — 显示辅助

### 4.1 TFT_eSPI 库简介

TFT_eSPI 是一个高性能 TFT 显示库，通过 `platformio.ini` 的 `build_flags` 配置引脚和驱动芯片，**不需要手动编辑库文件**。

关键配置：
```ini
-D ILI9341_DRIVER    # 使用 ILI9341 驱动
-D TFT_MOSI=16       # SPI 数据线
-D TFT_SCLK=18       # SPI 时钟
-D TFT_CS=4          # 片选
-D TFT_DC=5          # 数据/命令选择
-D TFT_RST=6         # 复位
-D TFT_MISO=15       # SPI 数据读取
-D USE_HSPI_PORT     # 使用 SPI3 外设
-D LOAD_GLCD         # 加载8像素字体
-D LOAD_FONT2        # 加载16像素字体
```

### 4.2 字体为什么之前不显示？

因为 `USER_SETUP_LOADED` 跳过了 TFT_eSPI 默认的 `User_Setup.h`，而该文件里定义了 `LOAD_GLCD`、`LOAD_FONT2` 等字体加载宏。跳过后字体没编译进去，`drawString()` 就什么都不显示。

**教训**：用 `build_flags` 覆盖配置时，**必须包含所有需要的宏**。

---

## 5. main.cpp — 应用逻辑

### 5.1 setup() — 初始化

```cpp
void setup() {
    Serial.begin(115200);   // 初始化串口，波特率115200
    delay(2000);            // 等待 USB CDC 就绪（ESP32-S3 需要）

    tft.init();             // 初始化 TFT_eSPI（配置 SPI3、发送 ILI9341 初始化序列）
    tft.setRotation(0);     // 竖屏模式 240×320
    tft.fillScreen(TFT_BLACK); // 清屏为黑色

    // 初始化触摸引脚
    pinMode(T_CS, OUTPUT);   digitalWrite(T_CS, HIGH);   // CS 默认高（不选中）
    pinMode(T_CLK, OUTPUT);  digitalWrite(T_CLK, LOW);   // 时钟默认低
    pinMode(T_SDI, OUTPUT);  digitalWrite(T_SDI, LOW);   // 数据默认低
    pinMode(T_SDO, INPUT_PULLUP);  // MISO 设为输入，加内部上拉

    tft.drawString("Touch the screen!", 5, 5);  // 显示提示文字
}
```

### 5.2 loop() — 主循环

```cpp
void loop() {
    uint16_t rawX = xptRead(0xD0);  // 读取触摸 X
    uint16_t rawY = xptRead(0x90);  // 读取触摸 Y
    uint16_t z = xptRead(0xB0);     // 读取压力 Z

    // 在屏幕上显示原始值
    tft.drawString("X:" + String(rawX), 5, 20);
    tft.drawString("Y:" + String(rawY), 5, 32);
    tft.drawString("Z:" + String(z), 5, 44);

    if (z > 50) {           // 压力超过阈值 = 触摸中
        tft.drawString("TOUCHED!", 5, 56);
        tft.fillCircle(120, 160, 5, TFT_GREEN);  // 画绿点
    } else {
        tft.drawString("no touch", 5, 56);
    }

    delay(100);  // 每100ms 读一次
}
```

---

## 6. 踩坑记录

### 坑1：GPIO 11/12/13 不能用
ESP32-S3 的 GPIO 11/12/13 连接内部 SPI Flash，用作普通 GPIO 会导致**黑屏甚至无法启动**。

### 坑2：GPIO 15/16/17/18 在某些板子上不在排针上
ESP32-S3-DevKitC-1 的 Octal PSRAM 版本会占用 GPIO 15-18。需要确认你的板子型号。

### 坑3：显示和触摸共享 SPI 总线冲突
ILI9341 模块的 SDO(MISO) 和 T_SDO 在模块内部**不连通**。如果只接一根 MISO 线，触摸芯片收不到数据。

**解决方案**：触摸使用独立引脚（T_SDI=21, T_SDO=47, T_CLK=14），通过 bit-bang SPI 通信。

### 坑4：`USE_HSPI_PORT` vs `USE_FSPI_PORT`
- `USE_HSPI_PORT`（SPI3）：本项目使用，**不能去掉**，否则崩溃
- `USE_FSPI_PORT`（SPI2）：在 ESP32-S3 上会导致崩溃
- 不加任何一个：也会崩溃（GPIO 冲突）

### 坑5：字体不显示
`USER_SETUP_LOADED` 跳过了默认配置，必须在 `build_flags` 里手动加 `-D LOAD_GLCD -D LOAD_FONT2`。

### 坑6：ESP32-S3 串口输出
ESP32-S3 需要 `-D ARDUINO_USB_CDC_ON_BOOT=1` 才能通过 USB 输出串口数据。

---

## 7. 学习路线

1. **理解 SPI 协议** → 看 `touchWrite()` 和 `touchRead12()`，理解时钟、数据线的配合
2. **理解 bit-bang** → 不用硬件 SPI 外设，手动翻转 GPIO 实现协议
3. **理解 CS 片选** → 多个 SPI 设备共享总线时，靠 CS 选择和谁通信
4. **理解 TFT_eSPI** → 通过 `build_flags` 配置库，不需要改源码
5. **理解模块化** → 把驱动、配置、应用分开，各司其职
6. **下一步** → 校准触摸坐标，实现手写绘图板
