/* phys_io.c */
/* physical I/O */
/* Quanser multiQ  */
/* Kihwal Lee, Dec 22, 2000     */

#include <multiq.h>
#include <sys/io.h>


/* For more information on each function, read:
 * - MultiQ driver for LynxOS
 * - MultiQ reference manual
 * - LS 7266 documentation from US Digital and www.boondog.com

 * Quanser MultiQ uses LS 7266 R1 encoder interface chip,
 * but is not directly accessed.  You must deal with multiplexed
 * registers for control.
*/


void da_initialize(void)
{

	int channel = 0, zerovolt = 2047;	/* volts/4096 + 2047 */
	if (ioperm(BASE_ADDR, 31, 1))
		perror("ioperm");


	outw(CONTROL_MUST | channel | AD_SEL_CHANNEL, BASE_ADDR + MQ_CONTROL);
	outw(zerovolt, BASE_ADDR + MQ_DA_DATA);
	outw(CONTROL_MUST, BASE_ADDR + MQ_CONTROL);
}

void da_write(unsigned int value)
{
	int channel = 0;

	outw(CONTROL_MUST | channel | AD_SEL_CHANNEL, BASE_ADDR + MQ_CONTROL);
	outw(value, BASE_ADDR + MQ_DA_DATA);
	outw(CONTROL_MUST, BASE_ADDR + MQ_CONTROL);
}

/* initialize the encoder ch0 and ch1 */
void ad_initialize(void)
{
	int channel = 0;
	for (; channel < 2; channel++) {
		outw(CONTROL_MUST | (channel << 3), BASE_ADDR + MQ_CONTROL);
		outw(EFLAG_RESET, BASE_ADDR + MQ_ENC_CTRL);
		outw(BP_RESET, BASE_ADDR + MQ_ENC_CTRL);
		outw(CLOCK_DATA, BASE_ADDR + MQ_ENC_DATA);
		outw(CLOCK_SETUP, BASE_ADDR + MQ_ENC_CTRL);
		outw(INPUT_SETUP, BASE_ADDR + MQ_ENC_CTRL);
		outw(QUAD_X4, BASE_ADDR + MQ_ENC_CTRL);
		outw(CNTR_RESET, BASE_ADDR + MQ_ENC_CTRL);
	}

}

/* read the encoder input */
void ad_read(unsigned int *ch0data, unsigned int *ch1data)
{
	int channel;
	unsigned int temp, chdata[2];

	for (channel = 0; channel < 2; channel++) {
		outb(AD_SH | AD_CLOCK_4M | AD_MUX_EN | (channel << 3),
		     BASE_ADDR + MQ_CONTROL);
		outb(BP_RESET, BASE_ADDR + MQ_ENC_CTRL);
		outb(TRSFRCNTR_OL, BASE_ADDR + MQ_ENC_CTRL);

		chdata[channel] = 0xff & inb(BASE_ADDR + MQ_ENC_DATA);
		chdata[channel] += (0xff & inb(BASE_ADDR + MQ_ENC_DATA)) << 8;
		temp = 0xff & inb(BASE_ADDR + MQ_ENC_DATA);
		if (temp & 0x80)
			temp = temp | 0xff00;
		chdata[channel] += temp << 16;

	}
	*ch0data = chdata[0];
	*ch1data = chdata[1];
}
