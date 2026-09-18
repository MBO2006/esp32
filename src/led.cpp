#include <Arduino.h>
#include "display.h"
#include "config.h"

void ledon()
{
    digitalWrite(LED_PIN,HIGH);
}

void ledoff()
{
    digitalWrite(LED_PIN,LOW);
}
