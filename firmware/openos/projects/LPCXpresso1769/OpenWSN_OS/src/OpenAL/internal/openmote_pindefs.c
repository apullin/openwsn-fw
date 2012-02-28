/*
 * openmote_pindefs.c
 *
 *  Created on: Feb 26, 2012
 *      Author: nerd256
 */

#include "openmote_pindefs.h"
#include "LPC17xx.h"



/*
 * 1 VCC
 * 2 P0[2]
 * 3 P0[3]
 * 4 P2[7]
 * 5 RESET
 * 6 P1[23]
 * 7 P1[24]
 * 8 P1[20]
 * 9 RESET
 * 10 GND
 * 11 P0[0]
 * 12 P2[0]
 * 13 P2[1]
 * 14 REFP
 * 15 P0[1]
 * 16 P2[10]
 * 17 P1[31]
 * 18 P1[30]
 * 19 P0[26]
 * 20 P0[25]
 */

const LPC_GPIO_TypeDef * LPC17xx_gpio_bank_addresses[OPENMOTE_NUM_GPIO_BANKS] = {
		LPC_GPIO0,
		LPC_GPIO1,
		LPC_GPIO2,
		LPC_GPIO3,
		LPC_GPIO4
};

const char openmote_pin_gpio_bank[OPENMOTE_NUM_PINS] = {
		NOT_VALID_PIN,
		0,
		0,
		2,
		NOT_VALID_PIN,
		1,
		1,
		1,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		0,
		2,
		2,
		NOT_VALID_PIN,
		0,
		2,
		1,
		1,
		0,
		0
};

const char openmote_pin_gpio_bit[OPENMOTE_NUM_PINS] = {
		NOT_VALID_PIN,
		2,
		3,
		7,
		NOT_VALID_PIN,
		23,
		24,
		20,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		0,
		0,
		1,
		NOT_VALID_PIN,
		1,
		10,
		31,
		30,
		26,
		25
};

const char openmote_uart_pins[OPENMOTE_NUM_UARTS][2] = {
		{2,3},
		{12,13},
		{20,19}
};

const LPC_UART_TypeDef * openmote_uart_bank_addresses[OPENMOTE_NUM_UARTS] = {
		(LPC_UART_TypeDef *)LPC_UART0,
		LPC_UART3,
		(LPC_UART_TypeDef *)LPC_UART1
};

const char openmote_i2c_pins[OPENMOTE_NUM_I2C][2] = {
		{11,15} // SDA, SCL
};

const LPC_I2C_TypeDef * openmote_i2c_bank_addresses[OPENMOTE_NUM_I2C] = {
		LPC_I2C1
};

const char openmote_spi_pins[OPENMOTE_NUM_SPI][3] = {
		{24,23,20}
}; // MOSI, MISO, SCK

const LPC_SSP_TypeDef * openmote_spi_bank_addresses[OPENMOTE_NUM_SPI] = {
		LPC_SSP0
};

const char openmote_pwm_channels[OPENMOTE_NUM_PINS] = {
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		4,
		5,
		2,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		1,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN
};

const char openmote_adc_channels[OPENMOTE_NUM_PINS] = {
		NOT_VALID_PIN,
		7,
		6,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		NOT_VALID_PIN,
		5,
		4,
		3,
		2
};
