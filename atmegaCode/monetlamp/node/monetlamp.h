#ifndef __AVR_MONETLAMP_H
#define __AVR_MONETLAMP_H

#include <avr/io.h>

#ifndef F_CPU
#define F_CPU (16000000UL)
#endif

#ifdef _AVR_IOM8_H_
	#define ATMEGA8
	#ifndef F_CPU
		#define F_CPU (16000000UL)
	#endif
#elif _AVR_IOM328P_H_
	#define ATMEGA328
	#ifndef F_CPU
		#define F_CPU (20000000UL)
	#endif
#else
	#error "NO CPU DEFINED"
#endif


#define MASTER 0x01
#define SLAVE1 0x02
#define SLAVE2 0x03

#define MASTER_ADDRESS	0x01
#define SLAVE1_ADDRESS	0x10
#define SLAVE2_ADDRESS	0x11

#if defined(ATMEGA328)
#define NODE MASTER
#else
#define NODE SLAVE1
#endif

#if NODE==MASTER
	#define NODE_ADDRESS MASTER_ADDRESS
#elif NODE==SLAVE1
	#define NODE_ADDRESS SLAVE1_ADDRESS
#elif NODE==SLAVE2
	#define NODE_ADDRESS SLAVE2_ADDRESS
#else
	#define NODE_ADDRESS 0x3F
#endif

#endif
