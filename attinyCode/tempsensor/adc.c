#include "adc.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define bit_set(byte,bit) byte |=  _BV(bit)
#define bit_clr(byte,bit) byte &= ~_BV(bit)

uint16_t ADC_result[ADC_PINS];
uint16_t  ADC_lines = 0;

void ADC_init(void) {
 // Vref: AVCC with external capacitor on AREF pin
 bit_set(ADMUX, REFS1);

 // Prescaler
 // ADC frequency should be 50-200Khz for max resolution. (datasheet)
 bit_set(ADC_CS_REGISTER, ADPS0);
 bit_set(ADC_CS_REGISTER, ADPS1);
 bit_set(ADC_CS_REGISTER, ADPS2);

 // Enable ADC
 bit_set(ADC_CS_REGISTER, ADEN);
}

void ADC_setEnabled(uint8_t lines) {
 ADC_lines = lines;
}

void ADC_start(void) {
 bit_set(ADC_CS_REGISTER, ADSC);
}

uint16_t ADC_get(uint8_t pin) {
 pin %= ADC_PINS;
 return ADC_result[pin];
}

// Used by scanning ADC
uint8_t ADC_result_i = 0;

void ADC_startScan(void) {
 if (!ADC_lines) return;
 for (ADC_result_i = 0; ADC_result_i < ADC_PINS; ADC_result_i++)
  if (bit_is_set(ADC_lines,ADC_result_i)) break; // We found the next one!

 ADC_setPin(ADC_result_i);
 bit_set(ADC_CS_REGISTER, ADIE); // Enable ISR
 ADC_start();
}

void ADC_stop(void) {
 bit_clr(ADC_CS_REGISTER, ADIE); // Disable ISR
}

void ADC_disable(void)
{
 bit_clr(ADC_CS_REGISTER, ADEN);
}

// Interrupt fired when AD conversion finished
ISR(ADC_vect) {
 ADC_result[ADC_result_i] = ADC;
 
 // Find the next result to get
 do {
  ADC_result_i++;
  ADC_result_i %= ADC_PINS;
 } while (bit_is_clear(ADC_lines,ADC_result_i));

 ADC_setPin(ADC_result_i);
 ADC_start();
}

void ADC_setPin (uint8_t pin) {
 // Make sure pin is in range 0-7
 pin %= ADC_PINS;

 pin  |= ADMUX & -8; // '-8' creates a bit mask to clear the lower 3 bits of ADMUX
 ADMUX = pin; // Good thing the binary values for pin #s fit straight into ADMUX
}

uint16_t ADC_readOnce(void) {
 bit_set(ADC_CS_REGISTER, ADEN);
 ADC_start();
 loop_until_bit_is_clear(ADC_CS_REGISTER, ADSC);
 loop_until_bit_is_clear(ADC_CS_REGISTER, ADSC);
 bit_clr(ADC_CS_REGISTER, ADEN);
 return ADC;
}
