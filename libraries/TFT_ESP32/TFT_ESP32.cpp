/*
 * TFT_ESP32.cpp - ILI9341 TFT Display Driver for ESP32
 * 实现文件
 */

#include "TFT_ESP32.h"

/* ===== 简易 5x8 字库 (ASCII 32-126) ===== */
static const uint8_t FONT_5X8[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x14,0x08,0x3E,0x08,0x14}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x00,0x41,0x22,0x14,0x08}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x10,0x08,0x08,0x10,0x08}, // '~'
};

/* ========== 构造函数 ========== */
TFT_ESP32::TFT_ESP32(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t blk)
    : _cs(cs), _dc(dc), _rst(rst), _blk(blk),
      _width(TFT_WIDTH), _height(TFT_HEIGHT),
      _cursorX(0), _cursorY(0),
      _textColor(COLOR_WHITE), _textBg(COLOR_BLACK),
      _textSize(1), _rotation(0),
      _spi(SPI)  // 使用默认 SPI
{
}

/* ========== 初始化 ========== */
void TFT_ESP32::begin(uint32_t spiFreq) {
    // 配置引脚
    pinMode(_cs, OUTPUT);
    pinMode(_dc, OUTPUT);
    pinMode(_rst, OUTPUT);
    pinMode(_blk, OUTPUT);

    digitalWrite(_cs, HIGH);
    digitalWrite(_blk, LOW);  // 先关背光

    // 初始化 SPI
    _spi.begin();
    _spi.beginTransaction(SPISettings(spiFreq, MSBFIRST, SPI_MODE0));

    // 硬件复位
    digitalWrite(_rst, HIGH);
    delay(10);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(150);

    // 发送初始化命令
    initDisplay();

    // 开背光
    setBacklight(true);
}

/* ========== ILI9341 初始化序列 ========== */
void TFT_ESP32::initDisplay() {
    writeCommand(ILI9341_SWRESET);
    delay(150);

    writeCommand(0xEF);
    writeData(0x03); writeData(0x80); writeData(0x02);

    writeCommand(0xCF);
    writeData(0x00); writeData(0xC1); writeData(0x30);

    writeCommand(0xED);
    writeData(0x64); writeData(0x03); writeData(0x12); writeData(0x81);

    writeCommand(0xE8);
    writeData(0x85); writeData(0x00); writeData(0x78);

    writeCommand(0xCB);
    writeData(0x39); writeData(0x2C); writeData(0x00); writeData(0x34); writeData(0x02);

    writeCommand(0xF7);
    writeData(0x20);

    writeCommand(0xEA);
    writeData(0x00); writeData(0x00);

    // 电源控制
    writeCommand(0xC0);
    writeData(0x23);

    writeCommand(0xC1);
    writeData(0x10);

    // VCOM 控制
    writeCommand(0xC5);
    writeData(0x3E); writeData(0x28);

    writeCommand(0xC7);
    writeData(0x86);

    // 显示方向 (横屏)
    writeCommand(ILI9341_MADCTL);
    writeData(0x28);  // MX | MV | BGR

    // 像素格式: 16bit
    writeCommand(ILI9341_PIXFMT);
    writeData(0x55);

    // 帧率控制
    writeCommand(0xB1);
    writeData(0x00); writeData(0x18);

    // 显示功能控制
    writeCommand(0xB6);
    writeData(0x08); writeData(0x82); writeData(0x27);

    // 退出睡眠
    writeCommand(ILI9341_SLPOUT);
    delay(150);

    // 开显示
    writeCommand(ILI9341_DISPON);
    delay(50);
}

/* ========== 底层 SPI 操作 ========== */
void TFT_ESP32::writeCommand(uint8_t cmd) {
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    _spi.transfer(cmd);
    digitalWrite(_cs, HIGH);
}

void TFT_ESP32::writeData(uint8_t data) {
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    _spi.transfer(data);
    digitalWrite(_cs, HIGH);
}

void TFT_ESP32::writeData16(uint16_t data) {
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    _spi.transfer(data >> 8);
    _spi.transfer(data & 0xFF);
    digitalWrite(_cs, HIGH);
}

void TFT_ESP32::setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    writeCommand(ILI9341_CASET);
    writeData16(x0);
    writeData16(x1);

    writeCommand(ILI9341_PASET);
    writeData16(y0);
    writeData16(y1);

    writeCommand(ILI9341_RAMWR);
}

/* ========== 显示控制 ========== */
void TFT_ESP32::setRotation(uint8_t r) {
    _rotation = r % 4;
    writeCommand(ILI9341_MADCTL);
    switch (_rotation) {
        case 0:
            writeData(0x48);  // MX | BGR
            _width = TFT_WIDTH;
            _height = TFT_HEIGHT;
            break;
        case 1:
            writeData(0x28);  // MV | BGR
            _width = TFT_HEIGHT;
            _height = TFT_WIDTH;
            break;
        case 2:
            writeData(0x88);  // MY | BGR
            _width = TFT_WIDTH;
            _height = TFT_HEIGHT;
            break;
        case 3:
            writeData(0xE8);  // MX | MY | MV | BGR
            _width = TFT_HEIGHT;
            _height = TFT_WIDTH;
            break;
    }
}

void TFT_ESP32::setBacklight(bool on) {
    digitalWrite(_blk, on ? HIGH : LOW);
}

void TFT_ESP32::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

/* ========== 画点 ========== */
void TFT_ESP32::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;
    setWindow(x, y, x, y);
    writeData16(color);
}

/* ========== 画线 ========== */
void TFT_ESP32::drawHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (y < 0 || y >= _height) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > _width) w = _width - x;
    if (w <= 0) return;

    setWindow(x, y, x + w - 1, y);
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    for (int16_t i = 0; i < w; i++) {
        _spi.transfer(color >> 8);
        _spi.transfer(color & 0xFF);
    }
    digitalWrite(_cs, HIGH);
}

void TFT_ESP32::drawVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (x < 0 || x >= _width) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > _height) h = _height - y;
    if (h <= 0) return;

    setWindow(x, y, x, y + h - 1);
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    for (int16_t i = 0; i < h; i++) {
        _spi.transfer(color >> 8);
        _spi.transfer(color & 0xFF);
    }
    digitalWrite(_cs, HIGH);
}

void TFT_ESP32::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Bresenham 画线算法
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* ========== 矩形 ========== */
void TFT_ESP32::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawHLine(x, y, w, color);
    drawHLine(x, y + h - 1, w, color);
    drawVLine(x, y, h, color);
    drawVLine(x + w - 1, y, h, color);
}

void TFT_ESP32::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= _width || y >= _height) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > _width)  w = _width - x;
    if (y + h > _height) h = _height - y;
    if (w <= 0 || h <= 0) return;

    setWindow(x, y, x + w - 1, y + h - 1);
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    uint32_t pixels = (uint32_t)w * h;
    for (uint32_t i = 0; i < pixels; i++) {
        _spi.transfer(color >> 8);
        _spi.transfer(color & 0xFF);
    }
    digitalWrite(_cs, HIGH);
}

/* ========== 画圆 (Bresenham) ========== */
void TFT_ESP32::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void TFT_ESP32::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    drawVLine(x0, y0 - r, 2 * r + 1, color);

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawVLine(x0 + x, y0 - y, 2 * y + 1, color);
        drawVLine(x0 - x, y0 - y, 2 * y + 1, color);
        drawVLine(x0 + y, y0 - x, 2 * x + 1, color);
        drawVLine(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

/* ========== 文字 ========== */
void TFT_ESP32::setCursor(int16_t x, int16_t y) {
    _cursorX = x;
    _cursorY = y;
}

void TFT_ESP32::setTextColor(uint16_t color) {
    _textColor = color;
    _textBg = color;  // 透明背景模式
}

void TFT_ESP32::setTextColor(uint16_t fg, uint16_t bg) {
    _textColor = fg;
    _textBg = bg;
}

void TFT_ESP32::setTextSize(uint8_t s) {
    _textSize = (s > 0) ? s : 1;
}

void TFT_ESP32::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 126) c = '?';

    const uint8_t *glyph = FONT_5X8[c - 32];

    for (int8_t col = 0; col < 5; col++) {
        uint8_t line = pgm_read_byte(&glyph[col]);
        for (int8_t row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                if (size == 1) {
                    drawPixel(x + col, y + row, color);
                } else {
                    fillRect(x + col * size, y + row * size, size, size, color);
                }
            } else if (bg != color) {
                if (size == 1) {
                    drawPixel(x + col, y + row, bg);
                } else {
                    fillRect(x + col * size, y + row * size, size, size, bg);
                }
            }
        }
    }
}

void TFT_ESP32::print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            _cursorX = 0;
            _cursorY += 8 * _textSize;
        } else {
            drawChar(_cursorX, _cursorY, *str, _textColor, _textBg, _textSize);
            _cursorX += 6 * _textSize;
            if (_cursorX > _width - 6 * _textSize) {
                _cursorX = 0;
                _cursorY += 8 * _textSize;
            }
        }
        str++;
    }
}

void TFT_ESP32::println(const char *str) {
    print(str);
    _cursorX = 0;
    _cursorY += 8 * _textSize;
}

/* ========== 颜色工具 ========== */
uint16_t TFT_ESP32::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
