/*
 *  es_comm.c
 *
 *  Embedded Simplex communication subsystem
 *  All the functions in this file are invoked only
 *  by server (eSimplex core).
 *
 *  Client functions are in es_comm_cli.c.
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
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <es_comm.h>


static struct es_msg *es_rpt, *es_st;
static struct es_msg *es_sub_srv, *es_sub_cli;
static void *es_shm_start;
static int period_miss = 0;
static int shmid, semid;



/*
 *  ini_es_comm()
 *
 *  Initializes the communication channel, in this case, shared memory.
 *  On Linux, the size of shm is rounded up to multitple of page size(4K?).
 *
 *  returns semid
 */
int ini_es_comm()
{
	struct sembuf mybuf[2] = { {0, 1, 0}, {1, 1, 0} };
	struct shmid_ds shmbuf;

	semctl(ES_SEM, 0, IPC_RMID);

	/* create shm */
	if ((shmid = shmget(ES_SHM, ES_SHM_SZ, IPC_CREAT | 0600)) < 0)
		perror("ini_es_comm: shmget");

	/* lock into mem to prevent from being swapped out */
	if (shmctl(shmid, SHM_LOCK, &shmbuf) < 0)
		perror("ini_es_comm: shmctl");

	/* attach to data pointers */
	if ((es_shm_start = shmat(shmid, 0, 0)) < 0)
		perror("ini_es_comm: shmat");

	es_rpt = (struct es_msg *) es_shm_start;
	es_st = es_rpt + 1;
	es_sub_srv = es_rpt + 2;
	es_sub_cli = es_rpt + 3;

	es_rpt->es_id = 0;
	es_st->es_id = 0;
	es_sub_cli->es_id = 0;
	es_sub_srv->es_id = 0;

	/* semaphores    */

	/* create a set of two semaphores
	 * index 0 : es_rpt and es_st
	 * index 1 : es_sub_srv and es_sub_cli
	 */
	if ((semid = semget(ES_SEM, 2, IPC_CREAT | 0600)) < 0)
		perror("ini_es_comm: semget");
	/* initialize both to 1 */
	if (semop(semid, mybuf, 2) < 0)
		perror("ini_es_comm: semop");

	return semid;
}



/*
 *  terminate_comm()
 *
 *  Destroys shared memory and semaphores.  Shared memory will actually be
 *  destroyed after all the processes detach it from its address space.
 *  It is invoked when eSimplex terminates
 */
void terminate_comm()
{
	shmdt(es_shm_start);
	shmctl(shmid, IPC_RMID, 0);
	semctl(semid, 2, IPC_RMID, 0);
}





/*
 *  process_sub()
 *
 *  Process subscription request from HPC .
 *  It will be invoked every calc phase, before making
 *  device status available.
 *  If the semaphore is busy for more than 2 periods,
 *  the HPC  will be terminated and semaphore
 *  value will be reset.
 *
 *  return value:
 *  0 if no subscription request.
 *  hpc_id if there was a subscription
 *  -1 if the semaphore is locked but request not completed
 *  -2 if hpc  is bad
 *
 */

int process_sub()
{
	struct sembuf lockbuf = { 1,	/* second semaphore in the set */
		-1,		/* try to get a lock */
		IPC_NOWAIT
	};			/* don't block */
	struct sembuf unlockbuf = { 1, 1, IPC_NOWAIT };	/* unlock */
	static int busycount = 0;

	/*  check if hpc has made a request.
	 *  they will lock the semaphore
	 */
	if (semop(semid, &lockbuf, 1) < 0) {
		if (errno != EAGAIN)
			perror("process_sub: semop");
		printf("subscription request received...\n");
		fflush(stdout);
		/* hpc has locked it
		 * we now have to see if hpc has made a request
		 */
		if (es_sub_srv->es_id == 0) {	/* request not made */
			if (busycount >= 2) {	/* bad hpc */
				busycount = 0;
				return -2;
			}	/* we will wait another round */
			busycount++;
			return -1;
		} else {	/* somebody wants to subscribe */

			es_sub_cli->es_id = es_sub_srv->es_id;
#ifdef DEBUG
			printf("subscribed by cx id:%d\n", es_sub_cli->es_id);
#endif
			printf("HPC subscribed: hpc_id=%d\n",
			       es_sub_cli->es_id);
			es_sub_srv->es_id = 0;	/* reset */

			/* the follwing is to be reset to 0 by hpc 
			 * after receiving the assigned id
			 */
			busycount = 0;
			return es_sub_cli->es_id;
		}
	} else {		/* no one locked the semaphore */

		if ((semop(semid, &unlockbuf, 1) < 0) && (errno != EAGAIN))
			perror("process_sub: semop");
		return 0;
	}
}

/*
 *  process_report()
 *
 *  No lock needed. If hpc couldn't write its control
 *  decision in time, it's a violation of timing requirement.
 *  It should have reported its control decision before
 *  eSimplex wakes up.  So no locking on the data structure
 *  is needed. Any incosistency leading to unsafe decision
 *  will be regarded as safety violation.
 *
 *  return values:
 *  0 : retrieved hpc 's report successfuly
 * -1 : bad hpc 
 * -2 : missing period
 */
int process_report(int hpc_id, float *volts)
{

	if (es_rpt->es_id == 0)	/* no report found */
		return -2;
	else if (es_rpt->es_id != hpc_id) {
		es_rpt->es_id = 0;	/* reset the report */
		return -1;
	} else {		/* normal behavior */
		*volts = es_rpt->es_command;
		es_rpt->es_id = 0;	/* reset */
		return 0;
	}
}

/*
 * publish_status()
 *
 * Publishes the device status to the communication channel.
 * ini_es_comm() must have been called before using this function.
 *
 * Unless a priority inversion occurs, eSimplex always publish
 * before hpc  reads it.  No locking is neccessary.
 * SCHED_FIFO and assigning higher priority to eSimplex also
 * ensure that eSimplex runs until its next sleep() or suspend(),
 * without giving any chance to hpc.
 * Check your OS scheduler if this condition still holds when
 * porting to a different OS.
 */
void publish_status(float track, float angle, int current_ctl, float volts)
{
	es_st->es_id = current_ctl;
	es_st->es_angle = angle;
	es_st->es_track = track;
	es_st->es_command = volts;
}
