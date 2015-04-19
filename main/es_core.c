/*
 *  es_core.c
 *
 *  Embedded Simplex core functions
 *
 *  Copyright (C) 2000, 2001 RTSL
 *
 *  Department of Computer Science
 *  University of Illinois at Urbana-Champaign
 *
 *  Kihwal Lee (klee7@cs.uiuc.edu)
 *  2000-12  Initial version
 *  2001-5   Reorganized for new architecture
 *  2005-11  modified for new architecture
 */

#include <ortga.h>
#include <phys_io.h>
#include <sys/types.h>
#include <signal.h>

#define __USE_ISOC99 1
#include <math.h>


static float x = 0.0, theta = 0.0;

extern float track, angle, volts;
extern int hpc_id;
float hac_volts;
//extern int process_report(int hpc_id, float *hpc_volts);

/*
 *  safety_test()
 *
 *  Safety test using LMI by Xue Liu (xuelue@cs.uiuc.edu)
 *
 */

static void safety_test_hac(float current_angle, float track_pos,
			    float local_volts, float *result)
{
	static float a[4][4] = { {37.62, 58.22, 17.87, 11.61},
	{58.22, 313.16, 69.36, 56.09},
	{17.87, 69.36, 29.81, 14.81},
	{11.61, 56.09, 14.81, 12.04}
	};

	float x_pre, theta_pre, xdot, thetadot;
//	static float x = 0.0, theta = 0.0;
	float xa[4];
	float temp[4] = { 0.0, 0.0, 0.0, 0.0 };
	int i, j;

	/*  This is the new LMI calculation from Xue Liu
	 *              ^^^  how new?
	 */
	x_pre = x;
	theta_pre = theta;
	x = track_pos / 100.0;
	theta = current_angle / 52.29578;	/* convert to rad */
	xdot = (x - x_pre) / (HAC_PERIOD / 1000.0);
	thetadot = (theta - theta_pre) / (HAC_PERIOD / 1000.0);

	/*  do the projection of the X_projection=Fx(k)+Gu(k)
	 *  First, get the voltage of input using the baseline (exp) controller.
	 */

	/* 0 : projected x
	 * 1 : projected theta
	 * 2 : projected xdot
	 * 3 : projected thetadot
	 */
	xa[0] = 1.0 * (x) + (-0.00051281) * (-theta) + 0.017961 * (xdot) +
		(-0.0000026781) * (-thetadot);
	xa[1] = 0 * (x) + 1.0056 * (-theta) + 0.0046419 * (xdot) +
		0.020029 * (-thetadot);
	xa[2] = 0 * (x) + (-0.049519) * (-theta) + 0.80322 * (xdot) +
		(-0.00043546) * (-thetadot);
	xa[3] = 0 * (x) + 0.55967 * (-theta) + 0.44824 * (xdot) +
		1.0048 * (-thetadot);

	xa[0] += 0.0003618 * local_volts;
	xa[1] += (-0.00082708) * local_volts;
	xa[2] += 0.034913 * local_volts;
	xa[3] += (-0.079879) * local_volts;

	/*  perform the actual check */
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			temp[i] = temp[i] + a[i][j] * xa[j];

	for (i = 0, *result = 0.0; i < 4; i++)
		*result += xa[i] * temp[i];
}

static void safety_test_hpc(float current_angle, float track_pos,
			    float local_volts, float *result)
{
	static float a[4][4] = { {37.62, 58.22, 17.87, 11.61},
	{58.22, 313.16, 69.36, 56.09},
	{17.87, 69.36, 29.81, 14.81},
	{11.61, 56.09, 14.81, 12.04}
	};

	float x_pre, theta_pre, xdot, thetadot;
//	static float x = 0.0, theta = 0.0;
	float xa[4];
	float temp[4] = { 0.0, 0.0, 0.0, 0.0 };
	int i, j;

	/*  This is the new LMI calculation from Xue Liu
	 *              ^^^  how new?
	 */
	x_pre = x;
	theta_pre = theta;
	x = track_pos / 100.0;
	theta = current_angle / 52.29578;	/* convert to rad */
	xdot = (x - x_pre) / (HPC_PERIOD / 1000.0);
	thetadot = (theta - theta_pre) / (HPC_PERIOD / 1000.0);

	/*  do the projection of the X_projection=Fx(k)+Gu(k)
	 *  First, get the voltage of input using the baseline (exp) controller.
	 */

	/* 0 : projected x
	 * 1 : projected theta
	 * 2 : projected xdot
	 * 3 : projected thetadot
	 */
	xa[0] = 1.0 * (x) + (-0.00051281) * (-theta) + 0.017961 * (xdot) +
		(-0.0000026781) * (-thetadot);
	xa[1] = 0 * (x) + 1.0056 * (-theta) + 0.0046419 * (xdot) +
		0.020029 * (-thetadot);
	xa[2] = 0 * (x) + (-0.049519) * (-theta) + 0.80322 * (xdot) +
		(-0.00043546) * (-thetadot);
	xa[3] = 0 * (x) + 0.55967 * (-theta) + 0.44824 * (xdot) +
		1.0048 * (-thetadot);

	xa[0] += 0.0003618 * local_volts;
	xa[1] += (-0.00082708) * local_volts;
	xa[2] += 0.034913 * local_volts;
	xa[3] += (-0.079879) * local_volts;

	/*  perform the actual check */
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			temp[i] = temp[i] + a[i][j] * xa[j];

	for (i = 0, *result = 0.0; i < 4; i++)
		*result += xa[i] * temp[i];
}


/*
 *  calc_hac()
 *
 *  Control command calculation for HAC
 *  the result is stored in hac_volts
 * input parameters: track, angle
 */
float calc_hac(void)
{
	static IP_State_t STS = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static IP_State_t FTS = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static IP_State_t PTS = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static IP_State_t SAS = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static IP_State_t FAS = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static IP_State_t PAS = { 0.0, 0.0, 0.0, 0.0, 0.0 };

	float voltsTrack, voltsAngle;
	float trackErrDot, angleErrDot;
	float trackErrMinus, trackPos, trackRef, angleErr;

	/* store track data */
	STS.previous_state = STS.current_state;
	STS.current_state = track;
	STS.current_derivative = (STS.current_state - STS.previous_state) *
		(1000 / HAC_PERIOD);

	/* filter track data */
	FTS.previous_previous_state = FTS.previous_state;
	FTS.previous_state = FTS.current_state;
	FTS.current_state = (STS.current_state * FILTER_WEIGHT) +
		(FTS.previous_state * (1.0 - FILTER_WEIGHT));
	FTS.previous_derivative = FTS.current_derivative;
	FTS.current_derivative = (FTS.current_state - FTS.previous_state) *
		(1000 / HAC_PERIOD);
	FTS.current_derivative = 0.5 * ((FTS.current_state -
					 FTS.previous_previous_state) *
					(1000 / HAC_PERIOD));

	/* project track data */
	PTS.previous_state = PTS.current_state;
	PTS.current_derivative = FTS.current_derivative;
	PTS.current_state = FTS.current_state + FTS.current_derivative *
		(HAC_PERIOD / 1000.0);

	/* store angle data */
	SAS.previous_state = SAS.current_state;
	SAS.current_state = angle;
	SAS.current_derivative = (SAS.current_state - SAS.previous_state) *
		(1000 / HAC_PERIOD);

	/* filter angle data */
	FAS.previous_previous_state = FAS.previous_state;
	FAS.previous_state = FAS.current_state;
	FAS.current_state = (SAS.current_state * FILTER_WEIGHT) +
		(FAS.previous_state * (1.0 - FILTER_WEIGHT));
	FAS.previous_derivative = FAS.current_derivative;
	FAS.current_derivative = 0.5 * (FAS.current_state -
					FAS.previous_previous_state) * (1000 /
									HAC_PERIOD);

	/* project angle data */
	PAS.previous_state = PAS.current_state;
	PAS.current_derivative = FAS.current_derivative;
	PAS.current_state = FAS.current_state + FAS.current_derivative *
		(HAC_PERIOD / 1000.0);

	/* calculate the voltage */
	trackPos = PTS.current_state;
	trackRef = STS.reference;
	trackErrMinus = trackPos - trackRef;
	trackErrDot = FTS.current_derivative;

	angleErr = PAS.current_state;
	angleErrDot = FAS.current_derivative;

	/* Balance control - stabilizing state feedback controller */
	voltsTrack = S_TGP * trackErrMinus + S_TGD * trackErrDot;
	voltsAngle = S_AGP * angleErr + S_AGD * angleErrDot;

	hac_volts = voltsAngle + voltsTrack;
	return hac_volts;

}
