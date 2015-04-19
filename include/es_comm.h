/*
 *  es_comm.h
 *
 *  Embedded Simplex communication subsystem
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
	
#define ES_SHM          100
#define ES_SEM          120
#define ES_SHM_SZ       2048	// will be rounded up to page size, most likely 4096.
struct es_msg  {
	int es_id;
	  float es_angle;
	  float es_track;
	  float es_command;
 };

/* shared memory map
 *
 * +--------+-------+------------+------------+
 * | es_rpt | es_st | es_sub_srv | es_sub_cli |
 * +--------+-------+------------+------------+
 *
 * - es_rpt     : complex controller writes its control command here.
 * - es_st      : simplex writes device status (i.e. angle & track) here.
 * - es_sub_srv : subscription channel. read by server. written by client.
 * - es_sub_cli : subscription channel. read by client. written by server.
 */ 
	
