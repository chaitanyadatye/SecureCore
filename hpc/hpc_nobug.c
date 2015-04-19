/*
 *  complex.c
 *
 *  Complex controller for eSimplex/IP
 *
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






/* Sampling frequency is fixed in eSimplex core
 * Changing following values won't affect actual
 * sampling frequency
 */
#define IP_FREQUENCY 50		/* 50Hz */
#define IP_PERIOD 0.02		/* 20ms */

#define FILTER_WEIGHT 1.0
#define MAX_VOLTAGE 4.96

/*   gains
 *   A|T : angle or track
 *   P|D : proportional or derivative
 */

#define C_AGP   -0.7307
#define C_AGD   -0.1501
#define C_TGP   0.0578
#define C_TGD   0.141

/*
#define C_AGP	-1.15
#define C_AGD	-0.153
#define C_TGP	0.18
#define C_TGD	0.172
*/

typedef struct {
	float current_state;
	float current_derivative;
	float previous_derivative;
	float previous_state;
	float previous_previous_state;
	float reference;
} IP_State_t;



/*
 *  void calc_command_cx(float angle, float track, float *volts)
 *
 *  This finction is to calculate the control command.  Users can write
 *  their own code, as long as the function prototype is identical.
 *
 *  input  : angle (in degrees), track (in centimeters)
 *  output : *volts
 *
 */
void calc_command_cx(float angle, float track, float *volts)
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

	*volts = cx_volts;
}
