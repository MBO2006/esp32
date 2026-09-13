/*
 * TFT_ESP32.h - ILI9341 TFT Display Driver for ESP32
 * 基于 SPI 的 ILI9341 驱动库
 */

#ifndef TFT_ESP32_H
#define TFT_ESP32_H

#include <Arduino.h>
#include <SPI.h>

/* ===== 屏幕分辨率 ===== */
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

/* ===== 默认引脚 (ESP32) ===== */
#define DEFAULT_CS   5
#define DEFAULT_DC   2
#define DEFAULT_RST  4
#define DEFAULT_BLK  15

/* ===== ILI9341 常用命令 ===== */
#define ILI9341_NOP        0x00
#define ILI9341_SWRESET    0x01
#define ILI9341_SLPOUT     0x11
#define ILI9341_DISPON     0x29
#define ILI9341_CASET      0x2A  // 列地址设置
#define ILI9341_PASET      0x2B  // 页地址设置
#define ILI9341_RAMWR      0x2C  // 写显存
#define ILI9341_MADCTL     0x36  // 显示方向
#define ILI9341_PIXFMT     0x3A  // 像素格式

/* ===== 常用颜色 (RGB565) ===== */
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F

class TFT_ESP32 {
public:
    /* 构造 / 初始化 */
    TFT_ESP32(uint8_t cs = DEFAULT_CS,
              uint8_t dc = DEFAULT_DC,
              uint8_t rst = DEFAULT_RST,
              uint8_t blk = DEFAULT_BLK);
    void begin(uint32_t spiFreq = 40000000);  // 默认 40MHz

    /* 显示控制 */
    void setRotation(uint8_t r);     // 0-3
    void setBacklight(bool on);
    void fillScreen(uint16_t color);

    /* 画点 */
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    /* 画线 */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

    /* 画矩形 */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    /* 画圆 */
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

    /* 文字 */
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t color);
    void setTextColor(uint16_t fg, uint16_t bg);
    void setTextSize(uint8_t s);
    void print(const char *str);
    void println(const char *str);
    void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);

    /* 颜色工具 */
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

private:
    uint8_t _cs, _dc, _rst, _blk;
    int16_t _width, _height;
    int16_t _cursorX, _cursorY;
    uint16_t _textColor;
    uint16_t _textBg;
    uint8_t  _textSize;
    uint8_t  _rotation;

    SPI &_spi;

    /* 底层 SPI 操作 */
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData16(uint16_t data);
    void setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    /* ILI9341 初始化序列 */
    void initDisplay();
};

#endif // TFT_ESP32_H
