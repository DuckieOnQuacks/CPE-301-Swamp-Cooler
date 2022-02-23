
#include <Arduino.h>
//Port B Registers
unsigned char* port_b = (unsigned char*) 0x25; 
unsigned char* ddr_b = (unsigned char*) 0x24; 


void setup() 
{
  Serial.begin(9600);
  //DC Motor Setup
  //Sets PB7 to output
  *ddr_b |= 0x01 << 7;
}

void loop() 
{
  delay(100);
  *port_b |= (0x01 << 7);
  delay(100);
  *port_b &= ~(0x01 << 7);
}
