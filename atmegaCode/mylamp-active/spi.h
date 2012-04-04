/*
 * spi.h
 *
 * Created: 28.3.2011 г. 11:56:53
 *  Author: mmirev
 */ 
#ifndef SPI_ATMEGA8A_H_
#define SPI_ATMEGA8A_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void spi_init( void );
extern void spi_putc( uint8_t );
extern void spi_start_sending( void );

#ifdef __cplusplus
}
#endif

#endif /* SPI_ATMEGA8A_H_ */