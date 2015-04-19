/*
 *  wrapper_cx.c
 *
 *  Embedded Simplex wrapper for complex controller
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

#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <sys/mman.h>

/* function prototypes */
extern void ini_es_comm_cx(void);
extern float calc_command_cx(float angle, float track);
extern void read_status_cx(float *angle, float *track);
extern void report_result_cx(float volts);
extern int subscribe_cx(void);


int main()
{
	float angle = 0.0, track = 0.0, volts = 0.0;
	struct sched_param sc;
	int stabilize = 0;

	sc.sched_priority = 5;
	if (sched_setscheduler(0, SCHED_FIFO, &sc))
		perror("sched_setscheduler");

	/* initialize the communication subsystem */
	ini_es_comm_cx();
	/* lock memory */
	mlockall(MCL_FUTURE);

	/* subscribe to eSimplex service */
	if (subscribe_cx() < 0) {
		printf("Complex controller: subscription failed\n");
		exit(-1);
	}

	/* computation loop:
	 * read_status_cx() is a blocking call.  Only one read is allowed
	 * in one period.  So this makes it a periodic task
	 */
	while (1) {

		read_status_cx(&angle, &track);	/* blocking read */

		/* the user supplied function */
		volts = calc_command_cx(angle, track);

		/* output small values until the filter stabilizes */
		if (stabilize < 2) {
			volts = 0.3;
			stabilize++;
		}

		report_result_cx(volts);
	}
}
