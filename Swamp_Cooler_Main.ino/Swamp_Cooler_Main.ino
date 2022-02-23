#include <RTClib.h>
#include <Arduino.h>
#include <Wire.h>
#include <dht_nonblocking.h>
#include <LiquidCrystal.h>

//Port B Registers
unsigned char* port_b = (unsigned char*) 0x25; 
unsigned char* ddr_b = (unsigned char*) 0x24;

// Port E Registers
unsigned char* const pin_e = (unsigned char*) 0x2C;
unsigned char* const port_e = (unsigned char*) 0x2E;
unsigned char* const ddr_e= (unsigned char*) 0x2D;

// Port H Registers Used for 4 LEDS
unsigned char* const port_h = (unsigned char*) 0x102;
unsigned char* const ddr_h = (unsigned char*) 0x101;

// Timer 1 Registers
volatile uint16_t* const tcnt_1 = (uint16_t*) 0x84;
unsigned char* const tccr1_a = (unsigned char*) 0x80;
unsigned char* const tccr1_b = (unsigned char*) 0x81;
unsigned char* const timsk_1 = (unsigned char*) 0x6F;

// Timer 3 Registers
volatile uint16_t* const tcnt_3 = (uint16_t*) 0x94;
unsigned char* const tccr3_a = (unsigned char*) 0x90;
unsigned char* const tccr3_b = (unsigned char*) 0x91;
unsigned char* const timsk_3 = (unsigned char*) 0x71;

// ADC Registers
volatile uint16_t* const adcValue = (uint16_t*) 0x78;
unsigned char* const admux = (unsigned char*) 0x7c;
unsigned char* const adcsr_a = (unsigned char*) 0x7a;
unsigned char* const adcsr_b = (unsigned char*) 0x7b;

// Interrupt Registers
unsigned char* const eimsk = (unsigned char*) 0x3D;
unsigned char* const eicrb = (unsigned char*) 0x6A;

// Enum that handles the different states.
enum State {Disabled, Idle, Running, Error} state;

// Creating an RTC clock object named clock
RTC_DS1307 clock;

// Initialize the LCD with pins listed 
LiquidCrystal lcd(37, 35, 33, 31, 29, 27);

// Threshold at which the fan is turned on
#define TEMPERATURE_DISABLE 75

<<<<<<< HEAD
// Amount of water necasarry to keep the cooler running
#define WATER_LEVEL_DISABLE 400
=======
// Amount of water neccasary to keep the cooler running
#define WATER_LEVEL_DISABLE 200
>>>>>>> 35cd244da2210d569562467726c42ee7133bcd3d

//Temp/Humidity Sensor Setup
#define DHT_SENSOR_PIN 14
#define DHT_SENSOR_TYPE DHT_TYPE_11
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

////////////////////////////////////////
// Timer1 overflowing interrupt handler
ISR(TIMER1_OVF_vect)
{
    *tccr1_b &= 0b11111000;   // Stop timer 1

    if(!(*pin_e & (1 << 4)))    // If pin is still low
    {
        if(state == Disabled) 
        {
          changeState(Idle);
        }
        else
        {
          changeState(Disabled);
        }
    }
}

////////////////////////////////////////////
// INT4 interrupt handler
// Starts Timer1 when the button is pressed
ISR(INT4_vect)
{
    *tcnt_1 = 34286;    // Starts timer
    *tccr1_b = (1 << 2);    // Shifts bits 2 to the left
}

/////////////////////////////
// Timer 3 interrupt handler
ISR(TIMER3_OVF_vect)
{
    *tccr3_b &= 0b11111000;  // Stop timer 3
    if(state == Running || state == Idle)
    {
        if(*adcValue <= WATER_LEVEL_DISABLE)    // If the water level is to low
        {
            changeState(Error);
        } 
    }
    else if (state == Error)
    {
      if(*adcValue > WATER_LEVEL_DISABLE)   // If the water level is back to the threshhold
      {
         changeState(Idle);
      }
    }
}

/////////////////////////////////////////////
// ADC conversion complete interrupt handler
ISR(ADC_vect)
{
    if(state == Running || state == Idle)
    {
        if(!timerRunning() && *adcValue <= WATER_LEVEL_DISABLE)    // If the water level is to low
        {
            *tcnt_3 = 34286;    // Starts timer
            *tccr3_b = (1 << 2);    // Shifts bits 2 to the left
        }
    } 
    else if (state == Error)
    {
        if(!timerRunning() && *adcValue > WATER_LEVEL_DISABLE)   // If the water level is back to the threshhold
        {
            *tcnt_3 = 34286;    // Starts timer
            *tccr3_b = (1 << 2);    // Shifts bits 2 to the left
        }
    }
    *adcsr_a |= (1 << 6);   // Start the next conversion
}

/////////////////////////////////////////////////////////////////////////////
void setup()
{   
    // Set Serial baud rate
    Serial.begin(9600);
   
    //LCD Screen Setup
    lcd.begin(16, 2);   // Setup the LCD's number of columns and rows:
    lcd.print("Disabled");
    
    // Initialize DS1307 (Real Time Clock)
    clock.begin();
    clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    //LEDS Setup
    *ddr_h = 0b01111000;    //Sets PH 3,4,5,6 to output
    setLEDColors(0);

    //DC Motor Setup
    *ddr_b |= 0x01 << 7;    //Sets PB7 to output
    
    // Enable the button
    buttonSetup();
    
    // ADC setup
    adcInit(0);
   
    // The program always begins in the disabled state
    state = Disabled;

    interrupts();
}

/////////////////////////////////////////////////////////////////////////////////
void loop()
{
    static float temp, humid;
    if(state == Running || state == Idle)
    {
        if(dht_sensor.measure(&temp, &humid)) 
        {
            noInterrupts();
            temp = temp * 9.0/5 + 32;   // Celsius to farenheit 

            lcd.setCursor(0, 1);
            lcd.print(temp, 1);
            lcd.print(" F");
            lcd.print(" - ");
            lcd.print(humid, 1); 
            lcd.print("%");

            interrupts();

            if((temp > TEMPERATURE_DISABLE) && (state == Idle))
            {
                changeState(Running);
            }
            else if((temp < TEMPERATURE_DISABLE) && (state == Running))
            {
                changeState(Idle);
            }
        }
    }

    // Only run the dc motor in the running state otherwise run the pin low
    if(state == Running) 
    {
       *port_b |= (0x01 << 7);
    }
    else
    {
       *port_b &= ~(0x01 << 7);
    }
    
    // Prints out water level
    if(state == Error)
    {
        noInterrupts();
        lcd.setCursor(0, 1);
        lcd.print("Water Left ");
        lcd.print(*adcValue, 1);
        interrupts();
    }
    //Serial.println(*adcValue);
    //delay(100);
}

////////////////////////////////
char stateCharacter(State state)
{
      if(state == Disabled)
      { 
        return 'D';
      }
      else if (state == Idle)
      {
        return 'I';
      }
      else if(state == Running)
      {
        return 'R';
      }
      else if(state == Error)
      {
        return 'E';
      }
      else
      {
        return 'U';
      }
}
///////////////////////////////////////////////////////
// Function which transitions from one state to another
void changeState(State alteredState)
{
    noInterrupts();
    if(state == Disabled)
    {
        adcInit(0);
    }
    
    switch(alteredState)
    {
    case Disabled:
              adcDisable();
              useLED(6);
              lcd.clear();
              lcd.print("Disabled");
        break;
        
    case Idle:
              useLED(5);
              lcd.clear();
              lcd.print("Idling");
        break;
        
    case Running:
              useLED(4);
              lcd.clear();
              lcd.print("Running Fan");
        break;
        
    case Error:
              useLED(3);
              lcd.clear();
              lcd.print("Error: No Water");
        break;
    }

    Serial.print(stateCharacter(state));
    state = alteredState;

    interrupts();
    
    Serial.print(" -> ");
    Serial.print(stateCharacter(state));
    Serial.print(" occurred at ");
    printTime();
}

////////////////////////////
void useLED(int pin)
{
    *port_h &= 0b10000111;    // Turn off LEDS
    *port_h |= (0x01 << pin);    // Shifts the bit based on what pin is used
}

///////////////////
void buttonSetup()
{
    *tccr1_a = 0;
    *tccr1_b = 0;
    *timsk_1 = 1;    // Enable the overflow interrupt
    *ddr_e = 0;    // Port is in input mode
    *port_e = (1 << 4);
    *eimsk = (1 << 4);    // Enable the interrupt on PE4
    *eicrb = 0b00000010;    // Set INT4 to be triggered when button is pressed
    *port_b &= ~(0x01 << 7);    //Turns off the DC motor when the disable button is pressed
}

/////////////////ADC STUFF///////////////////////
// Sets the required initial settings for the adc
void adcInit(uint8_t channel)
{
    *tccr3_b = 0;   // Enable the overflow interrupt
    *timsk_3 = 1; 
    *tccr3_a = 0; 
   
    *adcsr_a = 0b10001000;
    *admux = 0b01000000;
    *adcsr_b = 0;
    
    if(channel >= 8)    // If the selected channel is in the second port
    {
        channel -= 8;   // Drop off the leading bit from the channel
        *adcsr_b |= (1 << 3);   // Set the MUX bit in adcsr_b
    }
    *admux |= channel;
    *adcsr_a |= (1 << 6);
}

////////////////////
//Disables the ADC
void adcDisable()
{
    *adcsr_a = 0; // disable bit 7
}

///////////////////////////////////////////////////////
// Function which checks if the ADC's timer is running
bool timerRunning()
{
    return *tccr3_b &= 0b0000111;
}

/////////////////////////////////////////////////////////////////////////
// Function which prints the timestamp of when a state is changed
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
