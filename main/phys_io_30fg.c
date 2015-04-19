/* phys_io.c */
/* physical I/O */
/* For D/A conversion, Eagle Tech's PC104-30FG is used */
/* For device status reading, a LS7266R1 encoder chip is used */
/* Kihwal Lee, Dec 22, 2000     */



#include <30fg.h>		/* use Eagle Tech's PC104-30FG */
#include <ls7266.h>		/* use LS7266 encoder chip */
#include <sys/io.h>


void da_initialize(void)
{
	if (ioperm(BASE_ADDR, 31, 1))
		perror("ioperm");

	outb(0x00, BASE_ADDR + _30FG_IMUX);
	outb(0x00, BASE_ADDR + _30FG_IGATE);

	outb(0x00, BASE_ADDR + _30FG_GMEM0);
	outb(0x0F, BASE_ADDR + _30FG_DACCFG);	/* set the range to -5~+5 */

	/* write 0V = 2047 = 0x7FF */
	outb(0x7f, BASE_ADDR + AOUT_HI_OFFSET);
	outb(0xf0, BASE_ADDR + AOUT_LO_OFFSET);
}

/* D/A out for Eagle Tech's PC104-30FG */
void da_write(int value)
{
	int lowbyte, highbyte;

	lowbyte = 0xf0 & (value << 4);
	highbyte = 0xff & (value >> 4);

	/* writing lowbyte initiates d/a conversion */
	outb(highbyte, BASE_ADDR + AOUT_HI_OFFSET);
	outb(lowbyte, BASE_ADDR + AOUT_LO_OFFSET);
}

/* initialize the encoder chip */
/* read the LS7266 reference manual or */
/* the quadrature encoder card project at http://www.boondog.com */
void ad_initialize(void)
{
	if (ioperm(ENC_BASE_ADDR, 5, 1))
		perror("ioperm");

	outb(EFLAG_RESET, ENC_BASE_ADDR + ENC_CTRL_CH0);
	outb(BP_RESET, ENC_BASE_ADDR + ENC_CTRL_CH0);
	outb(CLOCK_DATA, ENC_BASE_ADDR + ENC_DATA_CH0);
	outb(CLOCK_SETUP, ENC_BASE_ADDR + ENC_CTRL_CH0);
	outb(INPUT_SETUP, ENC_BASE_ADDR + ENC_CTRL_CH0);
	outb(QUAD_X4, ENC_BASE_ADDR + ENC_CTRL_CH0);
	outb(CNTR_RESET, ENC_BASE_ADDR + ENC_CTRL_CH0);

	outb(EFLAG_RESET, ENC_BASE_ADDR + ENC_CTRL_CH1);
	outb(BP_RESET, ENC_BASE_ADDR + ENC_CTRL_CH1);
	outb(CLOCK_DATA, ENC_BASE_ADDR + ENC_DATA_CH1);
	outb(CLOCK_SETUP, ENC_BASE_ADDR + ENC_CTRL_CH1);
	outb(INPUT_SETUP, ENC_BASE_ADDR + ENC_CTRL_CH1);
	outb(QUAD_X4, ENC_BASE_ADDR + ENC_CTRL_CH1);
	outb(CNTR_RESET, ENC_BASE_ADDR + ENC_CTRL_CH1);


}

/* read the encoder input */

void ad_read(unsigned int *ch0data, unsigned int *ch1data)
{
	unsigned int temp = 0;

	/* read in ch 0 and ch 1                         */
	/* the values are 24 bit and registers are 8 bit */
	/* So we need to read three times.  read LS7266  */
	/* manual for detail                             */
	outb(BP_RESET, ENC_BASE_ADDR + ENC_CTRL_CH0);
	outb(TRSFRCNTR_OL, ENC_BASE_ADDR + ENC_CTRL_CH0);

	*ch0data = 0xff & inb(ENC_BASE_ADDR + ENC_DATA_CH0);
	*ch0data += (0xff & inb(ENC_BASE_ADDR + ENC_DATA_CH0)) << 8;

	temp = 0xff & inb(ENC_BASE_ADDR + ENC_DATA_CH0);
	if (temp & 0x80)
		temp = temp | 0xff00;
	*ch0data += temp << 16;


	outb(BP_RESET, ENC_BASE_ADDR + ENC_CTRL_CH1);
	outb(TRSFRCNTR_OL, ENC_BASE_ADDR + ENC_CTRL_CH1);

	*ch1data = 0xff & inb(ENC_BASE_ADDR + ENC_DATA_CH1);
	*ch1data += (0xff & inb(ENC_BASE_ADDR + ENC_DATA_CH1)) << 8;
	temp = 0xff & inb(ENC_BASE_ADDR + ENC_DATA_CH1);

	if (temp & 0x80)
		temp = temp | 0xff00;
	*ch1data += temp << 16;
}
