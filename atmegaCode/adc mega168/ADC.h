
#ifndef _ADC_H_
#define _ADC_H_

	/* Includes */
		#include <avr/io.h>
	
	/* Macros */
		#define ADC_PORT		PORTC
		#define ADC_DDR			DDRC
		#define ADC_HighByte		ADCH
		#define ADC_LowByte		ADCL
		#define ADC_NotComplete		!(ADSC)
			
		#define ADC_PowerReduce_Mask	(1 << 0)
		#define ADC_Enable_Mask		(1 << 7)
		#define ADC_Start_Mask		(1 << 6)
		#define ADC_LeftAdjust_Mask	(1 << 5)
		#define ADC_ClearChannel_Mask	(1 << 2) | (1 << 1) | (1 << 0)

	/* Function Prototypes */
		void ADC_Init(uint8_t Input_Mask);
		uint8_t ADC_Read8(uint8_t Channel);

	/* Inline Functions */
		static inline void ADC_SetupPins(uint8_t Input_Mask)
		{
			ADC_DDR = ~(Input_Mask);
			ADC_PORT = Input_Mask;	
		}
		
		static inline void ADC_Start(void) 
		{
			ADCSRA |= ADC_Start_Mask;
		}

		static inline void ADC_Stop(void) 
		{
			ADCSRA &= ~(ADC_Start_Mask);
		}
		
		static inline void ADC_Enable(void)
		{
			ADCSRA |= ADC_Enable_Mask;
		}
		
		static inline void ADC_Disable(void)
		{
			ADCSRA &= ~(ADC_Enable_Mask);
		}
		
		static inline void ADC_PowerReduceOn(void)
		{
			PRR |= ADC_PowerReduce_Mask;
		}
		
		static inline void ADC_PowerReduceOff(void)
		{
			PRR &= ~(ADC_PowerReduce_Mask);
		}
		
		static inline void ADC_ChangeChannel(uint8_t ADC_Channel)
		{
			ADMUX &= ~(ADC_ClearChannel_Mask);
			ADMUX |= ADC_Channel;
		}
		
		static inline void ADC_LeftAdjustOn(void)
		{
			ADMUX |= ADC_LeftAdjust_Mask;
		}
		
		static inline void ADC_LeftAdjustOff(void)
		{
			ADMUX &= ~(ADC_LeftAdjust_Mask);
		}
#endif
