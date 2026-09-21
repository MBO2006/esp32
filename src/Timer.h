/**
 * Time.h — 计时器接口
 */
#pragma once

#include <Arduino.h>

void startTimer();
void pauseTimer();
void resetTimer();
void AddTime();

int getTimerHours(void);
int getTimerMinutes(void);
int getTimerSeconds(void);
