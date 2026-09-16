/**
 * display.h — 显示辅助函数接口
 */
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * 按钮结构体
 */
struct Button
{
    uint16_t x, y, w, h; // 按钮位置和大小
    const char *label;   // 按钮标签
};

/**
 * 页面枚举
 */
enum Page
{
    PAGE_HOME,
    PAGE_FUNCTION,
    PAGE1,
    PAGE2,
    PAGE_COUNT
};

// 按钮声明（定义在 main.cpp）
extern Button home;
extern Button page1;
extern Button page2;
extern Button backBtn;
extern Button functionBtn;

// 显示
TFT_eSPI &getTft();
void displayInit();
void displayTouchInfo(bool touched);

// 页面绘制（每个函数负责清屏 + 画内容 + 画该页的按钮）
void drawPageHome(); // home

void drawpage1(); // page1
void drawPageFunction();

void drawpage2(); // page2

// 页面切换
void switchToHome();
void switchToFunction();

void gotopage1();

void gotopage2();

Page getCurrentPage();

// 按钮
void drawButton(const Button &btn);
bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn);
