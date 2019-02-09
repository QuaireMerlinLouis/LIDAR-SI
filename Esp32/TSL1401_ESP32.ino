// clock pin 11, CLK frequency : 7.27MHz
// SI pin 8

//https://github.com/hobgreenson

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 160000000UL
#define L_ARRAY 128
#define L_ARRAY_1 129

//global vars
uint8_t CLKcycleCounter = 0; // counts the number of clock cycles expired
volatile uint8_t pixels[L_ARRAY] = {0}; // pixel buffer - holds data from ADC
uint8_t i, send_pixels[L_ARRAY_1] = {0}; // i is an index for loops, m is the mimimum pixel value

// ADC interrupt routine - this happens when the ADC is finished taking a sample
ISR(ADC_vect) {
  // if AO is sending pixel data (first 128 clock cycles of each frame)
  // then do an ADC and put it in the pixel buffer
  if (CLKcycleCounter < L_ARRAY) {
    // start next CLK cycle
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB |= (1 << PINB3); // CLK = 5V
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB &= ~(1 << PINB3); // CLK = 0V
    pixels[CLKcycleCounter] = ADCH; // high byte of ADC data
    CLKcycleCounter++; // iterate clock cycle counter
  } else if (CLKcycleCounter == L_ARRAY | CLKcycleCounter == L_ARRAY_1) {
    // there is an extra clock cycle at the end (129) with no data
    // do a second extra clock for charge transfer
    // | CLKcycleCounter == L_ARRAY_1
    // start next CLK cycle
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB |= (1 << PINB3); // CLK = 5V
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB &= ~(1 << PINB3); // CLK = 0V
    CLKcycleCounter++; // iterate clock cycle counter
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
  } else { // start next frame by sending a pulse on SI
    PORTB |= (1 << PINB0); // SI = 5V
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB |= (1 << PINB3); // CLK = 5V
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB &= ~(1 << PINB0); // SI = 0V, must go low before next clock rise!!!
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    __asm__("nop\n\t"); // delay one CPU clock cycle = 12.5 nanosec
    PORTB &= ~(1 << PINB3); // CLK = 0V
    CLKcycleCounter = 0; // reset clock cycle counter to zero
  }
  ADCSRA |= (1 << ADSC); // start next conversion
}

int main(void) {
  // use serial library for communication with MATLAB
  Serial.begin(4000000);
  send_pixels[L_ARRAY] = 0xFF; // end of frame flag

  // set up digital output pins on port B; CLK = B3 (pin 11) and SI = B0 (pin 8)
  DDRB = 0; // zero the register
  PORTB = 0; // zero the register
  DDRB |= (1 << PINB3) | (1 << PINB0); // CLK(B3) and SI(B0)
  PORTB &= ~(1 << PINB0); // CLK = 0V
  PORTB &= ~(1 << PINB3); // SI = 0V

  // ADC set-up; ADC will read the analog output (AO) voltage from the sensor
  sei(); // enable global interrupts
  ADCSRA = 0; // zero the register
  ADCSRA |= (1 << ADEN); // turn on ADC
  ADCSRA |= (1 << ADPS2); // set clock to F_CPU/16 = 1Mhz
  ADCSRA |= (1 << ADIE); // enable ADC interrupt
  ADMUX |= (1 << ADLAR) | (1 << REFS0) | (1 << MUX2) | (1 << MUX0); // ADC is on pin 5;

  // start first frame
  PORTB |= (1 << PINB0); // SI = 5V
  __asm__("nop\n\t"); // delay one CPU clock cycle = 62.5 nanosec
  __asm__("nop\n\t"); // delay one CPU clock cycle = 62.5 nanosec
  PORTB |= (1 << PINB3); // CLK = 5V
  __asm__("nop\n\t"); // delay one CPU clock cycle = 62.5 nanosec
  __asm__("nop\n\t"); // delay one CPU clock cycle = 62.5 nanosec
  PORTB &= ~(1 << PINB0); // SI = 0V, must go low before next clock rise!!!
  __asm__("nop\n\t"); // delay one CPU clock cycle = 62.5 nanosec
  __asm__("nop\n\t"); // delay one CPU clock cycle = 62.5 nanosec
  PORTB &= ~(1 << PINB3); // CLK = 0V

  // first conversion
  ADCSRA |= (1 << ADSC);

  while (1) {
    // copy volatile array into non-volatile array
    for (i = 0; i < L_ARRAY; ++i) {
      send_pixels[i] = pixels[i];
    }
    // send the frame
    Serial.write(send_pixels, L_ARRAY_1);
  }
}


