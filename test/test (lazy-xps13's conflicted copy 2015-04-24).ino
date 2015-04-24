#include <EnableInterrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

#include <Time.h>

time_t currentTime;

void setup() {
    Serial.begin(9600);
    // put your setup code here, to run once:
    setTime(1429848335);
}

void loop() {
    Serial.println(getCurrentTime()); 

    delay(10000);
}

String getCurrentTime() {
    int temp;
    String t = String(year()) + "-";
    temp = month();
    if(temp < 9)
        t += "0";
    t += String(temp) + "-";
    temp = day();
    if(temp < 9)
        t += "0";
    t += String(temp) + "T";
    temp = hour();
    if(temp < 9)
        t += "0";
    t += String(temp) + ":";
    temp = minute();
    if(temp < 9)
        t += "0";
    t += String(temp);
    return t;
}