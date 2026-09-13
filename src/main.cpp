#include <Arduino.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#define T_CS   7
#define T_CLK  14
#define T_SDI  21
#define T_SDO  47

uint16_t xptRead(uint8_t cmd) {
    digitalWrite(T_CS, LOW);
    for (int i = 7; i >= 0; i--) {
        digitalWrite(T_SDI, (cmd >> i) & 1);
        digitalWrite(T_CLK, HIGH);
        delayMicroseconds(2);
        digitalWrite(T_CLK, LOW);
        delayMicroseconds(2);
    }
    delayMicroseconds(100);
    uint16_t val = 0;
    for (int i = 11; i >= 0; i--) {
        digitalWrite(T_CLK, HIGH);
        delayMicroseconds(2);
        val |= (digitalRead(T_SDO) << i);
        digitalWrite(T_CLK, LOW);
        delayMicroseconds(2);
    }
    digitalWrite(T_CS, HIGH);
    return val;
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Touch Demo ===");
    Serial.flush();

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    pinMode(T_CS, OUTPUT);   digitalWrite(T_CS, HIGH);
    pinMode(T_CLK, OUTPUT);  digitalWrite(T_CLK, LOW);
    pinMode(T_SDI, OUTPUT);  digitalWrite(T_SDI, LOW);
    pinMode(T_SDO, INPUT_PULLUP);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.drawString("Touch the screen!", 5, 5);

    Serial.println("READY");
    Serial.flush();
}

void loop() {
    uint16_t rawX = xptRead(0xD0);
    uint16_t rawY = xptRead(0x90);
    uint16_t z = xptRead(0xB0);

    // Show raw values
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("X:" + String(rawX) + "    ", 5, 20);
    tft.drawString("Y:" + String(rawY) + "    ", 5, 32);
    tft.drawString("Z:" + String(z) + "    ", 5, 44);

    if (z > 50) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("TOUCHED!", 5, 56);
        Serial.printf("rawX=%d rawY=%d z=%d\n", rawX, rawY, z);
        Serial.flush();
        // Draw a dot at a fixed position to prove touch works
        tft.fillCircle(120, 160, 5, TFT_GREEN);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("no touch", 5, 56);
    }

    delay(100);
}
