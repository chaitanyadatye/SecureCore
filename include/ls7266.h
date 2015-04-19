/* ls7266.h */
/* header file for LS7266R1 based encoder interface */
/* Kihwal Lee, Dec 22, 2000 */

#define ENC_BASE_ADDR 0x220
#define ENC_DATA_CH0 0
#define ENC_CTRL_CH0 1
#define ENC_DATA_CH1 2
#define ENC_CTRL_CH1 3


/* These definitions are from LS7266 example code */
/* See LS7266R1 reference manual for details */
#define CLOCK_DATA		0x14
#define CLOCK_SETUP		0x98
#define INPUT_SETUP		0xC1
#define QUAD_X4			0xB8
#define BP_RESET		0x81
#define CNTR_RESET		0x82
#define TRSFRCNTR_OL		0x90
#define EFLAG_RESET		0x86
