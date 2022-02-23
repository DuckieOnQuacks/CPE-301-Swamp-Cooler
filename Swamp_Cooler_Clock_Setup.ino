#include <RTClib.h>
#include <Arduino.h>
RTC_DS1307 clock;
void setup() 
{
    // Set Serial baud rate
    Serial.begin(9600);
    
    // Initialize DS1307
    clock.begin();
    clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() 
{
    Serial.print(" -> ");
    //Serial.print(state);
    Serial.print(" occurred at ");
    printTime();
}

void printTime()
{
    DateTime now = clock.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
}
