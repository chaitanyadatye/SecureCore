/*
 *  esimplex.c
 *
 *  Embedded Simplex main file
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
 *  2005-11  new architecture
 */
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <ortga.h>
#include <phys_io.h>

float angle = 0.0, track = 0.0, volts = 0.0;
int hpc_id = 0;			/* a non-zero val iff current controller is the hpc */

/* function prototypes */
extern float calc_hac(void);
extern void publish_status(float track, float angle, int current_ctl,
			   float volts);
extern int decision_hac(void);
extern int decision_hpc(void);


/* SIGALRM handler */
void sig_handler(int signum)
{
}


  /* read from ad interface
   * encoder 0: track, 1: angle
   */
static void phys_io_in(void)
{
	int raw_angle, raw_track, raw_volts;

	ad_read(&raw_track, &raw_angle);

	/* conversion to degrees and centimeters */
	track = TRACK_CONVERSION(raw_track);
	angle = ANGLE_CONVERSION(raw_angle);
}

static void phys_io_out(void)
{

	/* limit the voltage range to which DA converter can handle */
	if (volts > MAX_VOLTAGE)
		volts = MAX_VOLTAGE;
	if (volts < -MAX_VOLTAGE)
		volts = -MAX_VOLTAGE;

	/* turn off the motor when the IP is falling down */
	if (angle > 30.0 || angle < -30.0)
		volts = 0.0;

	da_write(VOLTS_TO_INT(volts));
}

/* returns 0 if succeeds
 *1 otherwise
 */
static int sanity_check(void)
{
	/* it doesn't have to be online check. it could be done
	 * in build time.
	 */
	if ((HAC_PERIOD >= ESIM_TICK_MS) &&
	    (HPC_PERIOD > ESIM_TICK_MS) &&
	    (HAC_PERIOD % ESIM_TICK_MS == 0) &&
	    (HPC_PERIOD % ESIM_TICK_MS == 0))
		return 0;
	else
		return 1;
}


int main()
{
	struct itimerval s;
	struct sigaction sa;
	struct sched_param sc;
	struct sembuf lockbuf = { 0,	/* first semaphore in the set */
		-1,		/* try to get a lock */
		IPC_NOWAIT
	};			/* don't block */
	struct sembuf unlockbuf = { 0, 1, IPC_NOWAIT };	/* unlock */

	int psub = 0, es_tick = 0;
	static int hac_pcount = 1, hpc_pcount = 1;
	static int semid;
	static int current_controller = ES_HAC;

	/* perform a sanity check on the parameters */
	if (sanity_check()) {
		printf("Error in configuration\n");
		return 1;
	}

	/* set scheduler and priority */
	sc.sched_priority = 10;
	if (sched_setscheduler(0, SCHED_FIFO, &sc))
		perror("sched_setscheduler");

	/* set the signal handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;
	if (sigaction(SIGALRM, &sa, NULL))
		perror("sigaction");

	/* initialize physical I/O */
	da_initialize();
	ad_initialize();

	/* initialize communication subsystem */
	semid = ini_es_comm();

	/* lock memory */
	mlockall(MCL_FUTURE);


	/* start the interval timer */
	s.it_value.tv_sec = 0;
	s.it_value.tv_usec = 100000;	/* start after 100ms :arbitrary */
	s.it_interval.tv_sec = 0;
	s.it_interval.tv_usec = ESIM_TICK_MS * 1000;	/* 10ms period */
	if (setitimer(ITIMER_REAL, &s, NULL))
		perror("settimer");


	semop(semid, &lockbuf, 1);
	while (1) {
		pause();	/* sleep until next timer event */
		es_tick++;

		if (current_controller == ES_HAC) {
			/* we do not deal with the message semaphore here.
			 * the subscription semaphore will be handled by
			 * the client
			 */
			if (hac_pcount == 1) {
				phys_io_out();
				/* get the plant state */
				phys_io_in();
				volts = calc_hac();
			}

			/* process hpc-join events. 
			 * it is performed in every subperiod
			 */
			if (hpc_id == 0 && (psub = process_sub()) > 0) {
				hpc_id = psub;
			}

			/* the last subperiod */
			if (hac_pcount == TICKS_PER_HAC_LOOP) {
				/* run decision, whether to switch to hpc.
				 * next time, we will be in the hpc loop */
				if (hpc_id != 0 && decision_hac()) {
					current_controller = ES_HPC;
					hpc_pcount = 1;
					printf("%d:Switching to HPC. (HAC suspended)\n\n",
					       es_tick);
				}
			}

			hac_pcount++;
			/* reset the counter */ ;
			if (hac_pcount > TICKS_PER_HAC_LOOP)
				hac_pcount = 1;
		} else {	/* HPC controller */

			/* The HPC must have a longer period than the system
			 * period.  The HAC can have the same period as the 
			 * system period.
			 *
			 * HAC_PERIOD >= ESIM_TICK_MS
			 * HPC_PERIOD > ESIM_TICK_MS
			 */


			/* The first subperiod */
			if (hpc_pcount == 1) {
				/* This is equivalent to the CALC phase in the old 
				 * eSimplex.  The first output will be actually from
				 * the hac
				 */
				phys_io_out();
				/* get the plant state */
				phys_io_in();
				
				publish_status(track, angle,
					       current_controller, volts);
					       
				/* allow the HPC's to run */
				semop(semid, &unlockbuf, 1);

			} /* this is the last loop */
			else if (hpc_pcount == TICKS_PER_HPC_LOOP) {
				/* block the hpc */
				semop(semid, &lockbuf, 1);

				/* run decision, whether to switch to hac. */
				if (decision_hpc()) {
					current_controller = ES_HAC;
					hac_pcount = 1;
					printf("%d:Switching to HAC\n",
					       es_tick);
			printf("------------------------------------------------------\n\n\n\n");				
}
				/* if we switch to the hac, the lock will be held
				 * until switching back to the hpc, blocking the
				 * execution of hpc.  We can also configure it
				 * to kill the HPC. See the config file.
				 */
			}
			/* we don't do any special thing in other subperiods */

			/* reset the counter */
			hpc_pcount++;
			if (hpc_pcount > TICKS_PER_HPC_LOOP)
				hpc_pcount = 1;

						}
	}
}
