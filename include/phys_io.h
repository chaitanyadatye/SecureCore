/*
 *  phys_io.h
 *
 *  Embedded Simplex physical IO header
 *
 *  Copyright (C) 2000, 2001 RTSL
 *
 *  Department of Computer Science
 *  University of Illinois at Urbana-Champaign
 *
 *  Kihwal Lee (klee7@cs.uiuc.edu)
 *  2000-12  Initial version
 *  2001-5   Reorganized for new architecture
 */


extern void da_initialize(void);
extern void da_write(unsigned int);
extern void ad_initialize(void);
extern void ad_read(unsigned int *, unsigned int *);

/* IP spec */
#define ENC_COUNT_PER_TURN 4096.0
#define ENC_COUNT_PER_CM 441.35
#define TRACK_RANGE 40		/* cm */

/* conversion macros */
#define TRACK_CONVERSION(x) (-(x)/ENC_COUNT_PER_CM)
#define ANGLE_CONVERSION(x) (-(x)*360/ENC_COUNT_PER_TURN)
#define VOLTS_TO_INT(x) (int)(x / 5.0 *2048 + 2047)
#define INT_TO_VOLTS(x) (((x - 2047)*5)/2048.0)
