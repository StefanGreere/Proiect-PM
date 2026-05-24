#include <avr/io.h>
#include <util/delay.h>
#include "spi.h"

void SPI_init(void) {
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
    
    /* Setăm MISO (PB4) ca intrare */
    DDRB &= ~(1 << PB4);

    /* SPE: Enable SPI, MSTR: Master Mode, SPR0: fosc/16 */
    SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

uint8_t SPI_exchange(uint8_t data){
	// TODO1: send a byte of data to the slave and return the response byte received from him in this transmission
	SPDR = data;
	 
	while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

