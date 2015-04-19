#ifndef _MultiQ_H_
#define _MultiQ_H_


/* base io address */

#define BASE_ADDR	0x320

/* Number of ADC channels on the multiq board */
#define MQ_NUMADC	8



/* Number of DAC channels on the multiq board */
#define MQ_NUMDAC	8



/* Number of encoder channels on the multiq board */
#define MQ_NUMENC	8


// registers on multiq3

#define MQ_DIN				0x00	//Digital input port offset

#define MQ_DOUT				0x00	//Digital output port offset

#define MQ_DA_DATA			0x02	//DA convertion port offset

#define MQ_AD_DATA			0x04	//AD convertion port offset

#define MQ_AD_CS			0x04

#define MQ_STATUS			0x06	// Status register offset(read)

#define MQ_CONTROL			0x06	// Control register offset(write)

#define MQ_CLKREG			0x08	// Clock data set offset (write)

#define MQ_ENC_DATA			0x0c	// Encoder data register offset

#define MQ_ENC_CTRL			0x0e	// Encode control register offset



// Bits in control registers

#define AD_SH			0x200	// Disable sample (Keep high all the time)

#define AD_AUTOCAL		0x100	// Enable autocalibration on A/D

#define AD_AUTOZ		0x080	// Enable auto Zero on A/D

#define AD_MUX_EN		0x040	// Enable the 8 channel multiplexer

#define AD_CLOCK_4M		0x400	// Select base clock frequency for A/D ( keep high )

#define AD_SEL_CHANNEL	0x1800	// Latch data to selected D/A channel when both high

#define AD_SEL_CLOCK	0x003	// Select the real-time clock register

#define CONTROL_MUST	(AD_SH|AD_CLOCK_4M)

unsigned int control_word = CONTROL_MUST;

// Bits in status register
#define AD_EOC			0x08	// End of conversion on the A/D

#define AD_EOC_I		0x10	// End of conversion interrupt, high when A/D conversion finished


// Encoder chip commands
#define CLOCK_DATA		0

#define CLOCK_SETUP		0x18	// set digital filter frequency

#define INPUT_SETUP		0x41	// select I/O control register and enable counter

#define QUAD_X4			0x38	// set quadrature counter mode

#define BP_RESET		0x01	// reset byte pointer

#define CNTR_RESET		0x02	// reset 24 bit counter to 0

#define TRSFRP_CNTR		0x08	// transfer preload to counter

#define TRSFRCNTR_OL	0x10	// transfer counter to output latch

#define EFLAG_RESET		0x06	// reset error



#define BASE_FREQ		2000000



// IRQ (jumpered to 3)

#define STAT_IRQ		5

#define PORT_LENGTH		8


#endif
