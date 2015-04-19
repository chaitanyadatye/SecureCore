/*
 *  ortga.h (i.e esimplex.h)
 *
 *  Embedded Simplex main header file
 *
 *  Copyright (C) 2000, 2001 RTSL
 *
 *  Department of Computer Science
 *  University of Illinois at Urbana-Champaign
 *
 *  Kihwal Lee (klee7@cs.uiuc.edu)
 *  2000-12  Initial version
 *  2001-5   Reorganized for new architecture
 *  2005-11  new architecture
 */

#ifndef _ESIMPLEX_H
#define _ESIMPLEX_H

/* -------------- User tunable settings ------------- */

/* The base period of simplex in milliseconds */
#define ESIM_TICK_MS 10

/* Controller periods.
 * See sanity_check() in es_core.c for restrictions
 */
#define HPC_PERIOD  30 
#define HAC_PERIOD   20

#define MAX_PERIOD_MISS 4

/* For calc_hac() */
#define FILTER_WEIGHT 1.0
#define S_AGP   -0.7578
#define S_AGD   -0.1500
#define S_TGP   0.0841
#define S_TGD   0.1192

/* hardware voltage limit */
#define MAX_VOLTAGE 4.96

/* safety value threasholds */
#define SAFETY_VAL_SWITCH_TO_HPC 5.0
#define SAFETY_VAL_HPC 5.0

/* Whether to kill a bad hpc. if 0, it is simply suspended
 * until we can switch back to it. if set to 1, the process
 * will be killed.
 */

#define KILL_BAD_HPC 0

/* ------------------------------------------------ */




typedef struct {
	float current_state;
	float current_derivative;
	float previous_derivative;
	float previous_state;
	float previous_previous_state;
	float reference;
} IP_State_t;

/* controller ID's */
#define ES_HAC       0
#define ES_HPC      2

#define TICKS_PER_HPC_LOOP (HPC_PERIOD/ESIM_TICK_MS)
#define TICKS_PER_HAC_LOOP (HAC_PERIOD/ESIM_TICK_MS)

#endif /* ortga.h */
