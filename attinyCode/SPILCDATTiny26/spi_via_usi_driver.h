/*
 * spi_via_usi_driver.h
 *
 *  Created on: 12.09.2008
 *      Author: dawg
 */
#ifndef SPI_VIA_USI_DRIVER_H_
#define SPI_VIA_USI_DRIVER_H_


/*! \brief  Driver status bit structure.
 *
 *  This struct contains status flags for the driver.
 *  The flags have the same meaning as the corresponding status flags
 *  for the native SPI module. The flags should not be changed by the user.
 *  The driver takes care of updating the flags when required.
 */
struct usidriverStatus_t {
	unsigned char masterMode : 1;       //!< True if in master mode.
	unsigned char transferComplete : 1; //!< True when transfer completed.
	unsigned char writeCollision : 1;   //!< True if put attempted during transfer.
};

extern volatile struct usidriverStatus_t spiX_status; //!< The driver status bits.

extern unsigned char spiX_put(unsigned char msg);
extern void spiX_initmaster(char msg);
extern void spiX_initslave(char msg);
extern void spiX_wait(void);
extern unsigned char spiX_get(void);

extern uint8_t spi_receive_buffer_not_empty( void );
extern uint8_t spi_receive_buffer_read( void );

#endif /* SPI_VIA_USI_DRIVER_H_ */