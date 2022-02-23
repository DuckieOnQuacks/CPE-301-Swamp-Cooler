#include <Arduino.h> 

// Port H Registers Used for 4 LEDS
unsigned char* const port_h = (unsigned char*) 0x102;
unsigned char* const ddr_h = (unsigned char*) 0x101;

void setup() 
{
    //LED
    *ddr_h = 0b01111000;        //Sets PH 3,4,5,6 to output
    setLED(0);
}

void loop() 
{
  setLED(6);
  setLED(5);
  setLED(4);
  setLED(3);
}

////////////////////
void setLED(int pin)
{
    *port_h &= 0b10000111;    // Turn off all of the LEDs
    *port_h |= (0x01 << pin);       // Turn on the LEDs specified in color
}
