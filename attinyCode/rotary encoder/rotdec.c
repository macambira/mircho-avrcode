/* Rotary encoder readout
 * Encoder on statusport 1 bits 5-7
 * GND on GND (pin 20)
 * Input patterns : cw: 1,0,2,6,7,5, ccw: 5,7,6,2,0,1
 */

#include <stdio.h>
#include <dos.h>
#include <conio.h>

int dataport, statusport, controlport;

int counter;

main(int argc, char *argv[])
{
	unsigned int encdat;
	unsigned int prevdat;
	unsigned int pattern;
	unsigned int counter;

	/* Initialize */
	dataport = 0x378;
	statusport = dataport + 1;
	controlport = dataport + 2;

	/* Main loop */
	prevdat = inportb(statusport) >> 5;
	counter = 0;
	while (!kbhit()) {
		encdat = inportb(statusport) >> 5;
		if (encdat != prevdat) {
			pattern = prevdat << 4 | encdat;
			switch (pattern) {
			case 0x10: case 0x02: case 0x26:
			case 0x67: case 0x75: case 0x51:
				counter++;
				break;
			case 0x57: case 0x76: case 0x62:
			case 0x20: case 0x01: case 0x15:
				counter--;
				break;
			default:
				printf("pattern = 0x%02x\n",pattern);
				exit(1);
			}
			printf("%05u\r",counter);
			prevdat = encdat;
		}
	}
	(void)getch();

	return(0);
}

