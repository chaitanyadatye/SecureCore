/*
 *  es_comm_cli.c
 *
 *  Embedded Simplex communication subsystem
 *  All the functions in this file are invoked only
 *  by client (i.e. complex controller).
 *
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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <es_comm.h>


static struct es_msg *es_rpt, *es_st;
static struct es_msg *es_sub_srv, *es_sub_cli;
static void *es_shm_start;
static int my_id = 0;
static int semid, shmid;


/*
 *  ini_es_comm_cx()
 *
 *  Initializes the communication channel, in this case, shared memory.
 *  On Linux, the size of shm is rounded up to multitple of page size(4K?).
 *
 */
void ini_es_comm_cx()
{
	struct sembuf mybuf[2] = { {0, 1, 0}, {1, 1, 0} };

	if ((shmid = shmget(ES_SHM, ES_SHM_SZ, 0600)) < 0)
		perror("ini_es_comm_cx: shmget");

	/* attach to data pointers */
	if ((es_shm_start = shmat(shmid, 0, 0)) < 0)
		perror("ini_es_comm_cx: shmat");

	es_rpt = (struct es_msg *) es_shm_start;
	es_st = es_rpt + 1;
	es_sub_srv = es_rpt + 2;
	es_sub_cli = es_rpt + 3;

	/*
	 * index 0 : es_rpt and es_st
	 * index 1 : es_sub_srv and es_sub_cli
	 */
	if ((semid = semget(ES_SEM, 2, 0600)) < 0)
		perror("ini_es_comm_cx: semget");

	my_id = getpid();
}



/*
 *  terminate_comm_cx()
 *
 * detach shared memory from its local addr space.
 */
void terminate_comm_cx()
{
	shmdt(es_shm_start);
}

/*
 *  report_result_cx()
 *
 *  report local decision to eSimplex
 */
void report_result_cx(float volts)
{

	es_rpt->es_id = my_id;
	es_rpt->es_command = volts;
}

/*
 *  read_status_cx()
 *
 *  read the device status published by esimplex
 *  the status variables are stored in es_msg
 */
void read_status_cx(float *angle, float *track)
{
	struct sembuf lockbuf = { 0,	/* first semaphore in the set */
		-1,		/* try to get a lock */
		0
	};			/* block */

	semop(semid, &lockbuf, 1);	/* try to lock, blocking */

	*angle = es_st->es_angle;
	*track = es_st->es_track;
}

/* for status reporter */
void read_status_cx2(float *angle, float *track, float *volts, int *cid)
{

	*angle = es_st->es_angle;
	*track = es_st->es_track;
	*volts = es_st->es_command;
	*cid = es_st->es_id;

}


/*
 *  subscribe_cx()
 *
 *  subscribe to eSimplex.  Ccomplex controller should acquire
 *  its ID before doing anything.  This ID is used whenever it
 *  reports its control decision to eSimplex
 *
 *  lock is used to prevent concuurent subscription requests.
 *  it won't happen in normal operations anyway.
 *  returns 0 on success or -1 if it fails
 *
 *  changes: my_id is my pid.  server uses my_id as complex
 *  controller's id
 */
int subscribe_cx()
{
	struct sembuf lockbuf = { 1,	/* second semaphore in the set */
		-1,		/* try to get a lock */
		0
	};			/* block */
	struct sembuf unlockbuf = { 1, 1, 0 };	/* unlock */
	int tries = 0;

	/* lock the semaphore */
	if (semop(semid, &lockbuf, 1) < 0)
		perror("subscrice_cx: semop");

	/* subscription request
	 * for semantics of each var, see es_comm.h
	 */
	es_sub_srv->es_id = my_id;	/* my pid */
	printf("my_id is %d\n", my_id);

	/* wait until my request is processed */
	tries = 0;
	printf("Waiting for subscription approval...");
	fflush(stdout);
	while (1) {
		usleep(10000);
		/* get the assigned id and unlock */
		if (es_sub_cli->es_id == my_id) {
			es_sub_cli->es_id = 0;
			if (semop(semid, &unlockbuf, 1) < 0)
				perror("subscrice_cx: semop");
			printf("  approved!\n");
			fflush(stdout);
			return 0;
		}
		tries++;
	}

	es_sub_cli->es_id = 0;
	if (semop(semid, &unlockbuf, 1) < 0)
		perror("subscrice_cx: semop");
	return -1;

}
