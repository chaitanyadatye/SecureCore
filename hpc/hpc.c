/*
 *  hpc.c
 *
 *  High Performance Controller (HPC) for ORTGA/IP
 *
 *
 *  Copyright (C) 2000, 2001, 2006 RTSL
 *
 *  Department of Computer Science
 *  University of Illinois at Urbana-Champaign
 *
 *  
 *  2000-12    Initial version
 *  2001-5     Reorganized for new architecture
 *  2005-2006  Redesigned and reorganized for ORTGA architecture 
 */


/* Sampling frequency is fixed in eSimplex core
 * Changing following values won't affect actual
 * sampling frequency
 */
#include <ortga.h>
#define IP_FREQUENCY (1000/HPC_PERIOD)	
#define IP_PERIOD (HPC_PERIOD/1000.0)

#define FILTER_WEIGHT 1.0
#define MAX_VOLTAGE 4.96

/*   gains
 *   A|T : angle or track
 *   P|D : proportional or derivative
 */

#define C_AGP   -0.7578
#define C_AGD   -0.1501
#define C_TGP   0.0841
#define C_TGD   0.1192

/*
#define C_AGP	-1.15
#define C_AGD	-0.153
#define C_TGP	0.18
#define C_TGD	0.172
*/

/* we are including eximplex.h so we do not need the following.
typedef struct {
	float current_state;
	float current_derivative;
	float previous_derivative;
	float previous_state;
	float previous_previous_state;
	float reference;
} IP_State_t;
*/


/*
 *  float calc_command_cx(float angle, float track)
 *
 *  This finction is to calculate the control command.  Users can write
 *  their own code, as long as the function prototype is identical.
 *
 *  input  : angle (in degrees), track (in centimeters)
 *  output : *volts
 *
 */
float calc_command_cx(float angle, float track)
{
	static IP_State_t STS = { 0.0, 0.0, 0.0, 0.0, 0.0 };	/* S: stored */
	static IP_State_t FTS = { 0.0, 0.0, 0.0, 0.0, 0.0 };	/* F: Filtered */
	static IP_State_t PTS = { 0.0, 0.0, 0.0, 0.0, 0.0 };	/* P: Projected */
	static IP_State_t SAS = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static IP_State_t FAS = { 0.0, 0.0, 0.0, 0.0, 0.0 };	/* T: Track */
	static IP_State_t PAS = { 0.0, 0.0, 0.0, 0.0, 0.0 };	/* A: Angle */
	static int glob_ctr = 0;

	float voltsTrack, voltsAngle;
	float trackErrDot, angleErrDot;
	float trackErrMinus, trackPos, trackRef, angleErr;
	float cx_volts;

	/* store track data */
	STS.previous_state = STS.current_state;
	STS.current_state = track;
	STS.current_derivative = (STS.current_state - STS.previous_state) *
		IP_FREQUENCY;

	/* filter track data */
	FTS.previous_previous_state = FTS.previous_state;
	FTS.previous_state = FTS.current_state;
	FTS.current_state = (STS.current_state * FILTER_WEIGHT) +
		(FTS.previous_state * (1.0 - FILTER_WEIGHT));
	FTS.previous_derivative = FTS.current_derivative;
	FTS.current_derivative =
		(FTS.current_state - FTS.previous_state) * IP_FREQUENCY;
	FTS.current_derivative =
		0.5 * ((FTS.current_state - FTS.previous_previous_state) *
		       IP_FREQUENCY);


	/* project track data */
	PTS.previous_state = PTS.current_state;
	PTS.current_derivative = FTS.current_derivative;
	PTS.current_state =
		FTS.current_state + FTS.current_derivative * IP_PERIOD;

	/* store angle data */
	SAS.previous_state = SAS.current_state;
	SAS.current_state = angle;
	SAS.current_derivative =
		(SAS.current_state - SAS.previous_state) * IP_FREQUENCY;

	/* filter angle data */
	FAS.previous_previous_state = FAS.previous_state;
	FAS.previous_state = FAS.current_state;
	FAS.current_state = (SAS.current_state * FILTER_WEIGHT) +
		(FAS.previous_state * (1.0 - FILTER_WEIGHT));
	FAS.previous_derivative = FAS.current_derivative;
	FAS.current_derivative =
		0.5 * (FAS.current_state -
		       FAS.previous_previous_state) * IP_FREQUENCY;

	/* project angle data */
	PAS.previous_state = PAS.current_state;
	PAS.current_derivative = FAS.current_derivative;
	PAS.current_state =
		FAS.current_state + FAS.current_derivative * IP_PERIOD;

	/* calculate the voltage */
	trackPos = PTS.current_state;
	trackRef = STS.reference;
	trackErrMinus = trackPos - trackRef;
	trackErrDot = FTS.current_derivative;

	angleErr = PAS.current_state;
	angleErrDot = FAS.current_derivative;

	voltsTrack = C_TGP * trackErrMinus + C_TGD * trackErrDot;
	voltsAngle = C_AGP * angleErr + C_AGD * angleErrDot;

	cx_volts = voltsAngle + voltsTrack;

	/* limit the voltage range so that da converter can handle it.
	 * Complex controller don't have to do this but it will get killed anyway
	 * if the value is not acceptable.  To reduce the chance of getting killed,
	 * check volts before reporting it.
	 */
	if (cx_volts > MAX_VOLTAGE)
		cx_volts = MAX_VOLTAGE;
	if (cx_volts < -MAX_VOLTAGE)
		cx_volts = -MAX_VOLTAGE;
/* ------------------------------------------------------------ */
	/* Predefined bugs.
	 *
	 * You may use it or create you own to test
	 * the robustness of telelab
	 *
	 * To activate a bug, remove the comment(//)
	 * in front of #define BUG_NAME
	 */

/*****************************************************************/
	/* Infinite loop
	 *   Performs normal operation for a while (50 * 5 * 20ms = 5 secs)
	 *   then go into an infinite loop
	 */

//#define INFINITE_LOOP_BUG
#ifdef INFINITE_LOOP_BUG
	if (glob_ctr++ > 600) 
		while (1) {
		};
#endif


/*****************************************************************/
	/* Non performing
	 *   After few seconds of (5 secs) of normal operation,
	 *   outputs 0 voltage
	 */

//#define NON_PERF_BUG
#ifdef NON_PERF_BUG
	if (glob_ctr++ > 600)
		cx_volts = 0.0;

#endif



/*****************************************************************/

//#define DIVIDED_BY_ZERO_BUG
#ifdef DIVIDED_BY_ZERO_BUG

	if (glob_ctr++ > 300)
		cx_volts = 1.0/0.0;

#endif


/*****************************************************************/
//#define POSITIVE_FEEBACK_BUG
#ifdef POSITIVE_FEEBACK_BUG

	if (glob_ctr++ > 300)
		cx_volts = -cx_volts;

#endif



/*****************************************************************/

	/*  MAX Control Bug( the maximum voltage is +/- 5 V)
	 *    After few seconds of normal operation, output
	 *    maximum voltages
	 */


//#define MAX_CONTROL_BUG
#ifdef MAX_CONTROL_BUG

	if (glob_ctr++ > 600)
		cx_volts = 5.0;

#endif


/*****************************************************************/

//#define BANG_BANG_TYPE_BUG
#ifdef BANG_BANG_TYPE_BUG

	if (glob_ctr++ > 300)
		cx_volts = 5.0;
	if (glob_ctr > 50 * 10)
		cx_volts = -5.0;

#endif


/*****************************************************************/
	/* Wrong way
	 *    After few seconds of normal operation, output
	 *    maximum voltage to the opposite direction
	 */

//#define WRONG_WAY_BUG
#ifdef WRONG_WAY_BUG

	if (glob_ctr++ > 600) {
		if (angle < 0.0)
			cx_volts = 5.0;
		else
			cx_volts = -5.0;
	}
#endif



/*****************************************************************/

	/* Tricky designer bug
	 *    A sofisticated attack created by a control engineer from Honeywell
	 *    
	 */

//#define TRICKEY_DESIGNER_BUG
#ifdef TRICKEY_DESIGNER_BUG
	{
		static float wdither = 0.0;
		static int wratio = 1;
		static float wincremental = 0.30;

		glob_ctr++;

		if ((glob_ctr % 800) == 0) {
			/* wratio = 2; */
			wincremental += 0.80;
			wdither = wincremental;
		}

		if ((glob_ctr % 100) == 0) {
			wratio += 10;
		}

		if ((glob_ctr % wratio) == 0) {
			if (wdither == wincremental)
				wdither = -wincremental;
			else if (wdither == -wincremental)
				wdither = wincremental;
		}

		cx_volts += wdither;
	}
#endif


	return cx_volts;
}
