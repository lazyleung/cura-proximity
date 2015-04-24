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
    setTime(1429847135);
}

void loop() {
    // currentTime = now();
    // String startTime = year(currentTime) + "-";
    // startTime += month(currentTime) + "-";
    // startTime += day(currentTime) + "T";
    // startTime += hour(currentTime) + ":";
    // startTime += minute(currentTime);
    // Serial.println(startTime);

    // Serial.print(hour());
    // printDigits(minute());
    // printDigits(second());
    // Serial.print(" ");
    // Serial.print(day());
    // Serial.print(" ");
    // Serial.print(month());
    // Serial.print(" ");
    // Serial.print(year()); 
    Serial.println(getCurrentTime()); 

    delay(10000);
}

String printDigits(int digits){
  if(digits < 10)
    return "0" + String(digits);
  return digits;
}

String getCurrentTime(){
    String t = year() + "-";
    t += printDigits(month());
    t += month() + "-";
    t += day() + "T";
    t += hour() + ":";
    t += minute();
    Serial.println(t);
}