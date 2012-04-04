


#include "ADC.h"

/** Returns 8-bit ADC value */
uint8_t ADC_Read8(uint8_t ADC_Channel) 
{
	ADC_LeftAdjustOn();
	ADC_ChangeChannel(ADC_Channel);
	ADC_Start();
	while(ADC_NotComplete);
	return(ADC_HighByte);
}
	
/** Initializes the ADC hardware */
void ADC_Init(uint8_t Input_Mask) 
{
	ADC_SetupPins(Input_Mask);
	ADC_PowerReduceOff();
	ADC_Enable();
}


