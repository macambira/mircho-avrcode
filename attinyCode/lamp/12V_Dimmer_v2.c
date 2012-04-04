// 12V_Dimmer_v2.c
// 2nd generation of 12V Dimmer project
// dale wheat - 27 march 2010

// notes:

// device:  ATtiny13 or ATtiny13A
// clock:   9.6 MHz internal RC oscillator, shortest startup time
// brownout detect:  2.7V

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

// These registers are available on the ATtiny13A but not the original ATtiny13

// Brown out detector control register

#define BODCR _SFR_IO8(0x30)
#define BODSE 0
#define BODS 1

// Power reduction register

#define PRR _SFR_IO8(0x3C)
#define PRADC 0
#define PRTIM0 1

///////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////

#define nop() asm("nop")

#define sbi(port, bit) (port) |= (1 << (bit))
#define cbi(port, bit) (port) &= ~(1 << (bit))

///////////////////////////////////////////////////////////////////////
// system constants
///////////////////////////////////////////////////////////////////////

#define PWM_MAX (248 << 8)
#define DIAL_HYSTERESIS (10 << 8)

///////////////////////////////////////////////////////////////////////
// global variables
///////////////////////////////////////////////////////////////////////

typedef enum {
	MODE_DIAL,
	MODE_BUTTON,
} MODE;

//volatile MODE mode __attribute__ ((section(".noninit"))); // operational mode, survives resets
volatile unsigned char mode; // can't watch defined types in debugger

///////////////////////////////////////////////////////////////////////
// EEPROM non-volatile storage section
///////////////////////////////////////////////////////////////////////

//unsigned char eeprom_do_not_use EEMEM; // bad luck - do not use location 0
//unsigned char eeprom_mode EEMEM; // mode storage location
//unsigned char eeprom_button_index EEMEM; // last known button index

enum {
	eeprom_do_not_use, // bad luck - do not use location 0
	eeprom_mode, // mode storage location
	eeprom_dial_trigger_low,
	eeprom_dummy_1,
	eeprom_dial_trigger_high,
	eeprom_dummy2,
	eeprom_button_index, // last known button index
};

///////////////////////////////////////////////////////////////////////
// debug functions
///////////////////////////////////////////////////////////////////////

#define DEBUG_STROBE_PORT PORTB
#define DEBUG_STROBE_PIN PB2

//void debug_strobe() {
//
//	sbi(DEBUG_STROBE_PORT, DEBUG_STROBE_PIN); // *** debug *** strobe output
//	cbi(DEBUG_STROBE_PORT, DEBUG_STROBE_PIN);
//}

//#define debug_strobe() sbi(DEBUG_STROBE_PORT, DEBUG_STROBE_PIN); cbi(DEBUG_STROBE_PORT, DEBUG_STROBE_PIN) // will work when optimzed
#define debug_strobe() asm("sbi 0x18, 0x02"); asm("cbi 0x18, 0x02") // pre-optimized

///////////////////////////////////////////////////////////////////////
// adc() - read analog value: Vcc=0, gnd=PWM_MAX, returns 0 if no potentiometer installed
///////////////////////////////////////////////////////////////////////

#define VCC_PORT PORTB
#define VCC_PIN PB3

#define vcc_on() sbi(VCC_PORT, VCC_PIN)
#define vcc_off() cbi(VCC_PORT, VCC_PIN)

unsigned int adc(void) {

	unsigned int value; // return value

	vcc_on(); // apply Vcc to potentiometer

	// configure on-chip analog-to-digital converter

	ADMUX = 0<<REFS0 | 1<<ADLAR | 1<<MUX1 | 0<<MUX0; // REF = Vcc, left adjust, ADC2/PB4 selected
	ADCSRA = 1<<ADEN | 0<<ADSC | 0<<ADATE | 1<<ADIF | 1<<ADIE | 1<<ADPS2 | 1<<ADPS1 | 0<<ADPS0;

	// go to sleep, wake when conversion complete

	set_sleep_mode(SLEEP_MODE_IDLE); // "SLEEP_MODE_ADC" shuts down PWM clock, so use "IDLE" mode instead
	sleep_enable();
	sleep_cpu();

	// conversion is complete

	value = ADC; // get converted analog value from ADC
	value = ~value; // invert bits
	value &= 0xFFC0; // mask 10 significant bits
	//value = ~ADC & 0xFFC0; // read converted value; invert, truncate to significant bits
/*
	if(mode == MODE_DIAL) {

		if(value != 0) {

			// first conversion is always an "extended" (i.e., calibration) conversion
			// for power off settings (dial=0), we don't care, but for active modes we should perform another conversion

			ADMUX = 0<<REFS0 | 1<<ADLAR | 1<<MUX1 | 0<<MUX0; // REF = Vcc, left adjust, ADC2/PB4 selected
			ADCSRA = 1<<ADEN | 0<<ADSC | 0<<ADATE | 1<<ADIF | 1<<ADIE | 1<<ADPS2 | 1<<ADPS1 | 0<<ADPS0;

			// go to sleep, wake when conversion complete

			set_sleep_mode(SLEEP_MODE_IDLE); // "SLEEP_MODE_ADC" shuts down PWM clock, so use "IDLE" mode instead
			sleep_enable();
			sleep_cpu();

			// conversion is complete

			value = ~ADC & 0xFFC0; // read converted value; invert, truncate to significant bits
		}
	}
*/
	vcc_off(); // remove Vcc from potentiometer

	// disable ADC
	// disable ADC conversion complete interrupt
	// clear pending conversion conplete interrupt, if any

	ADCSRA = 0<<ADEN | 0<<ADSC | 0<<ADATE | 1<<ADIF | 0<<ADIE | 1<<ADPS2 | 1<<ADPS1 | 0<<ADPS0;

	return value;
}

///////////////////////////////////////////////////////////////////////
// button_state() - return state of push button
///////////////////////////////////////////////////////////////////////

#define BUTTON_PORT PINB
#define BUTTON_PIN PB0

enum {
	BUTTON_RELEASED,
	BUTTON_PRESSED
};

unsigned char button_state(void) {

	if(bit_is_clear(BUTTON_PORT, BUTTON_PIN)) {
		return BUTTON_PRESSED; // button pressed, it seems
	} else {
		return BUTTON_RELEASED; // button not pressed, it seems
	}
}

///////////////////////////////////////////////////////////////////////
// pwm() - set PWM output value
///////////////////////////////////////////////////////////////////////

void pwm(unsigned int value) {

	if(value >> 8 == 0) {

		// special case 0; turn off PWM output

		cbi(PORTB, PB1); // set output to low level (should already be, but)
		//sbi(PORTB, PB1); // set output to high level (should already be, but)
		//TCCR0A = 0<<COM0A1 | 0<<COM0A0 | 0<<COM0B1 | 0<<COM0B0 | 1<<WGM01 | 1<<WGM00; // PWM off
		TCCR0A = 0<<COM0A1 | 0<<COM0A0 | 0<<COM0B1 | 0<<COM0B0 | 0<<WGM01 | 0<<WGM00; // PWM off
		//TCCR0B = 0<<FOC0A | 0<<FOC0B | 1<<WGM02 | 0<<CS02 | 0<<CS01 | 0<<CS00; // PWM clock = off
		TCCR0B = 0<<FOC0A | 0<<FOC0B | 0<<WGM02 | 0<<CS02 | 0<<CS01 | 0<<CS00; // PWM clock = off

	} else {

		// normal case

		//OCR0A = PWM_MAX; // set TOP value for PWM output:  composite of min reading from ADC with pullup (~252) and limitations of averaging aglorithm (+/- 1)
		//OCR0A = PWM_MAX >> 8; // set TOP value for PWM output:  composite of min reading from ADC with pullup (~252) and limitations of averaging aglorithm (+/- 1)
		OCR0A = (PWM_MAX >> 8) - 1; // set TOP value for PWM output:  composite of min reading from ADC with pullup (~252) and limitations of averaging aglorithm (+/- 1)
		OCR0B = value >> 8; // normal case, simply set value
		TCCR0A = 0<<COM0A1 | 0<<COM0A0 | 1<<COM0B1 | 0<<COM0B0 | 1<<WGM01 | 1<<WGM00; // PWM B on
		//TCCR0A = 0<<COM0A1 | 0<<COM0A0 | 1<<COM0B1 | 1<<COM0B0 | 1<<WGM01 | 1<<WGM00; // PWM B on, inverted
		//TCCR0B = 0<<FOC0A | 0<<FOC0B | 1<<WGM02 | 0<<CS02 | 0<<CS01 | 1<<CS00; // PWM clock = CK/1
		TCCR0B = 0<<FOC0A | 0<<FOC0B | 1<<WGM02 | 1<<CS02 | 0<<CS01 | 0<<CS00; // PWM clock = CK/256
	}
}

///////////////////////////////////////////////////////////////////////
// main() - main program function
///////////////////////////////////////////////////////////////////////

void main(void) {

	// system-level variables

	unsigned int brightness = 0; // how bright my light is
	int proportional_error = 0, derivative_error = 0; // filter terms
	int error;

	// dial mode variables

	unsigned int dial_reading;

	unsigned int dial_trigger_low, dial_trigger_high; // escape from button mode values

	// button mode variables

	//unsigned char button_value; // to measure duration of button press
	unsigned char button_pressed = 0; // flag indicates button is currently believed to be pressed
	unsigned char button_released = 1; // flag indicates that button is believed to be released

	unsigned char button_press_event = 0; // button press event flag
	//unsigned char button_release_event = 0; // button release event flag

	//const unsigned char button_preset[5] = { 0, 25, 65, 125, 250 }; // 0%, 10%, 26%, 50%, 100%
	//const unsigned int button_preset[] = { 0, 256, 6553, 16384, 32768, PWM_MAX }; // 0%, 1%, 10%, 25%, 50%, 100%
	const unsigned int button_preset[] = { 0, 0x100, 6349, 15872, 31744, PWM_MAX }; // 0%, min visible, 10%, 25%, 50%, 100%
	const int button_step_rate[] = { 12 << 8, 1 << 8, 1 << 8, 2 << 8, 3 << 8, 5 << 8 }; // linear step rates
	//const unsigned int button_step_rate[5] = { 12 << 8, 1 << 8, 2 << 8, 3 << 8, 5 << 8 }; // linear step rates
	unsigned char button_index = 0;

	// initialize everything

	// turn off unused peripherals to save power

	ACSR = 1<<ACD; // disable analog comparator
	DIDR0 = 1<<ADC3D | 1<<ADC2D | 1<<ADC1D | 1<<ADC0D | 1<<AIN1D | 0<<AIN0D; // disable unused digital inputs on ADC/AC (except PB1/AIN1/INT0)

	// initialize input & output port PORTB

	//	PB0		5		MOSI/AIN0/OC0A/PCINT0		pushbutton input
	//	PB1		6		MISO/AIN1/OC0B/INT0/PCINT1	PWM output B
	//	PB2		7		SCK/ADC1/T0/PCINT2			debug output
	//	PB3		2		PCINT3/CLKI/ADC3			Vcc for potentiometer
	//	PB4		3		PCINT4/ADC2					analog input for potentiometer
	//	PB5		1		PCINT5/-RESET/ADC0/dW		RESET button (maybe)

	PORTB = 1<<PORTB5 | 1<<PORTB4 | 0<<PORTB3 | 0<<PORTB2 | 1<<PORTB1 | 1<<PORTB0;
	DDRB = 0<<DDB5 | 0<<DDB4 | 1<<DDB3 | 1<<DDB2 | 1<<DDB1 | 0<<DDB0;

	// initialize ATtiny13 external & pin change interrupts

	PCMSK = 0<<PCINT5 | 0<<PCINT4 | 0<<PCINT3 | 0<<PCINT2 | 0<<PCINT1 | 1<<PCINT0;
	//GIMSK = 0<<INT0 | 1<<PCIE; // enable pin change interrupt

	sei(); // enable global interrupts

	// restore saved values from EEPROM

	mode = eeprom_read_byte((const uint8_t *) eeprom_mode);
	//if((mode != MODE_DIAL) && (mode != MODE_BUTTON)) mode = MODE_DIAL; // default, explicit
	if(mode != MODE_BUTTON) mode = MODE_DIAL; // default, slightly more optimized

	//dial_trigger_low = eeprom_read_byte((const uint8_t *) eeprom_dial_trigger_low);
	dial_trigger_low = eeprom_read_word((const uint16_t *) eeprom_dial_trigger_low);
	//dial_trigger_high = eeprom_read_byte((const uint8_t *) eeprom_dial_trigger_high);
	dial_trigger_high = eeprom_read_word((const uint16_t *) eeprom_dial_trigger_high);

	button_index = eeprom_read_byte((const uint8_t *) eeprom_button_index);
	if(button_index >= (sizeof(button_preset) / sizeof(button_preset[0]))) button_index = 0; // default

	// button state snapshot

	//if(button_state() == BUTTON_RELEASED) {
	//	button_pressed = 0; // it seems to be not pressed at the moment
	//	button_released = 1; // it seems to be released at the moment
	//	//button_value = 0; // not currently pressed, or ever pressed
	//} else {
	//	button_pressed = 1; // it seems to be pressed at the moment
	//	button_released = 0; // it seems to be not released at the moment
	//	//button_value = 1; // a little bit pressed, but not much
	//}

	// the big loop

	while(1) {

		//debug_strobe(); // *** debug *** strobe the debug output
		
		// get information from potentiometer

		dial_reading = adc(); // get new input value

		//if(dial_reading < DIAL_HYSTERESIS) {
		//	dial_reading = 0; // clip lower limit
		//} else if(dial_reading > PWM_MAX) {
		//	dial_reading = PWM_MAX; // clip high limit
		//}

		// determine if button events (button press or button release) have occurred

		if(button_released) {
			if(button_state() == BUTTON_PRESSED) {
				button_press_event = 1; // a button press event has occurred
				button_pressed = 1; // pressed
				button_released = 0; // not released
			}
		} else if(button_pressed) {
			if(button_state() == BUTTON_RELEASED) {
				//button_release_event = 1; // a button release event has occurred
				button_pressed = 0; // not pressed
				button_released = 1; // released
			}
		}

		switch(mode) {

			case MODE_DIAL: // dial mode active - responds to input potentiometer

				if(button_press_event) {

					//button_press_event = 0; // consume event
					mode = MODE_BUTTON; // shift to button mode
					eeprom_write_byte((uint8_t *) eeprom_mode, mode); // commit to memory

					// set trigger points to escape button mode via dial adjust

					if(dial_reading >= DIAL_HYSTERESIS) {
						dial_trigger_low = dial_reading - DIAL_HYSTERESIS;
					} else {
						dial_trigger_low = 0; // avoid going negative here
					}
					eeprom_write_word((uint16_t *) eeprom_dial_trigger_low, dial_trigger_low); // commit to memory
					
					//if(dial_reading < (PWM_MAX - DIAL_HYSTERESIS)) {
					//	dial_trigger_high = dial_reading + DIAL_HYSTERESIS;
					//} else {
					//	dial_trigger_high = PWM_MAX;
					//}
					if(dial_reading < ~DIAL_HYSTERESIS) {
						dial_trigger_high = dial_reading + DIAL_HYSTERESIS;
					} else {
						dial_trigger_high = 0xFFFF;
					}
					eeprom_write_word((uint16_t *) eeprom_dial_trigger_high, dial_trigger_high); // commit to memory

					continue; // start over at the top of the big loop
				}

				// some basic debugging values to try

				//brightness++; // *** debug ***
				//brightness += 8; // *** debug ***
				//brightness += 32; // *** debug ***

				// PD filter

				error = dial_reading - brightness; // compute error term
				
				//proportional_error = error << 3; // Kp = 8
				//proportional_error = error; // Kp = 1
				
				proportional_error = error >> 2; // Kp = 1/4;  normal

				//if(brightness > 2048) {
				//	proportional_error = error >> 2; // Kp = 1/4;  normal
				//} else {
				//	proportional_error = error >> 1; // Kp = 1/2;  small values
				//}
				
				brightness += (proportional_error + derivative_error); // sum error terms
				
				//derivative_error = proportional_error << 3; // Kd = 8Kp
				//derivative_error = proportional_error; // Kd = Kp
				derivative_error = proportional_error >> 1; // Kd = 1/2
				//derivative_error = proportional_error >> 2; // Kd = 1/4
				//derivative_error = proportional_error >> 4; // Kd = 1/16
		
				// direct assignment
				
				//brightness = dial_reading;

				// averaging algorithm
				
				//brightness += ((dial_reading - brightness) >> 1);
				
				// averaging algorithm, take 2
				//error = dial_reading - brightness;
				//brightness += error >> 4;

				break;

			case MODE_BUTTON: // 6-click dimmer mode

				// mode decision

				if((dial_reading < dial_trigger_low) || (dial_reading > dial_trigger_high)) {
					mode = MODE_DIAL; // switch back to dial mode
					eeprom_write_byte((uint8_t *) eeprom_mode, mode); // commit to memory
					continue; // start over at the top of the big loop
				}

				// button press action

				if(button_press_event) {
					button_press_event = 0; // consume event
					button_index++; // advance button preset index
					if(button_index >= sizeof(button_preset) / sizeof(button_preset[0])) button_index = 0; // rollover
					eeprom_write_byte((uint8_t *) eeprom_button_index, button_index); // commit to memory
				}

				error = (button_preset[button_index] >> 1) - (brightness >> 1); // distance from target

				if(error > button_step_rate[button_index]) {
					brightness += button_step_rate[button_index];
				} else if(error < -button_step_rate[button_index]) {
					brightness -= button_step_rate[button_index];
				} else {
					brightness = button_preset[button_index]; // close enough - get there fast
				}

				break;
		}

		pwm(brightness); // set PWM level

		// prepare for a variable period of sleep:  16ms for normal, 250ms for sleepy

		wdt_reset(); // reset the watchdog timer
		if(brightness > 255) {
			// a short nap while the output is somewhat on
			WDTCR = 0<<WDTIF | 0<<WDTIE | 0<<WDP3 | 1<<WDCE | 1<<WDE | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; // enable changes
			WDTCR = 1<<WDTIF | 1<<WDTIE | 0<<WDP3 | 0<<WDCE | 0<<WDE | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; // new values: 16ms
			set_sleep_mode(SLEEP_MODE_IDLE);
			//sleep_enable();
		} else {
			// shut down as much as possible when the output is completely off
			WDTCR = 0<<WDTIF | 0<<WDTIE | 0<<WDP3 | 1<<WDCE | 1<<WDE | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; // enable changes
			WDTCR = 1<<WDTIF | 1<<WDTIE | 0<<WDP3 | 0<<WDCE | 0<<WDE | 1<<WDP2 | 0<<WDP1 | 0<<WDP0; // new values: 250ms
			GIFR = 1<<INTF0 | 1<<PCIF; // clear pending interrupts
			GIMSK = 0<<INT0 | 1<<PCIE; // enable pin change interrupt for instant wake-up on button-press
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			//BODCR = 1>>BODS | 1>>BODSE; // prepare to disable the brown out detector during power down sleep mode
			//BODCR = 1>>BODS | 0>>BODSE; // disable brown out detector
			//MCUCR = 1<<SE | 1<<SM1 | 0<<SM0; // enable power down sleep mode
		}

		// go to sleep; wait for interrupts

		sleep_enable();
		sleep_cpu();

		// recover from deep sleep

		GIMSK = 0<<INT0 | 0<<PCIE; // disable pin change interrupt
	}
}

///////////////////////////////////////////////////////////////////////
// pin-change interrupt handler
///////////////////////////////////////////////////////////////////////

EMPTY_INTERRUPT(PCINT0_vect);

///////////////////////////////////////////////////////////////////////
// timer/counter0 overflow interrupt handler
///////////////////////////////////////////////////////////////////////

//EMPTY_INTERRUPT(TIM0_OVF_vect);

///////////////////////////////////////////////////////////////////////
// analog-to-digital converter interrupt handler
///////////////////////////////////////////////////////////////////////

EMPTY_INTERRUPT(ADC_vect);

///////////////////////////////////////////////////////////////////////
// watchdog timer interrupt handler
///////////////////////////////////////////////////////////////////////

EMPTY_INTERRUPT(WDT_vect);

///////////////////////////////////////////////////////////////////////

// [end-of-file]
