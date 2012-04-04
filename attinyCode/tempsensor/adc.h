#ifndef _ADC_H_
#define _ADC_H_

#include <inttypes.h>

#define ADC_PINS 11
#define ADC_CS_REGISTER	ADCSR

// Initialize the ADC
void ADC_init(void);

// Sets the current pin in the MUX settings
void ADC_setPin (uint8_t pin);

// Sets which lines are enabled from a bit mask
void ADC_setEnabled(uint8_t lines);

// Get the most recent value from pin
uint16_t ADC_get(uint8_t pin);

// Read (block) the value from the currently selected pin
uint16_t ADC_readOnce(void);

// Start a single convertion
void ADC_start(void);

// Start the scanning, free running, adc
void ADC_startScan(void);

// Stop it
void ADC_stop(void);

// Disable it, saves power
void ADC_disable(void);


#endif //_ADC_H_
