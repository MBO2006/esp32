/**
 * main.cpp — LVGL 智能手表主程序
 */
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "touch.h"
#include "display.h"
#include "lvgl_setup.h"
#include "screens/ui_screens.h"
#include "loop.h"

/* ===== WiFi 配置 ===== */
#define WIFI_SSID "wmyj"
#define WIFI_PASS "20060814"

/* NTP 服务器（中国时区 UTC+8） */
#define NTP_SERVER "ntp.aliyun.com"
#define GMT_OFFSET_SEC (8 * 3600)
#define DST_OFFSET_SEC 0

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(2000);
    Serial.println("=== LVGL Smartwatch ===");
    Serial.flush();

    /* 硬件初始化 */
    displayInit();
    touchInit();
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    /* 连接 WiFi */
    Serial.printf("连接 WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi 已连接! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi 连接失败，继续运行");
    }

    /* 同步 NTP 时间 */
    configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
    Serial.println("NTP 时间同步中...");

    /* LVGL 初始化 */
    lvgl_setup_init();

    /* 创建界面 */
    screens_init();

    /* 注册回调 */
    app_init_callbacks();

    Serial.println("=== READY ===");
    Serial.flush();
}

void loop()
{
    lvgl_setup_loop();
    delay(5);
}
