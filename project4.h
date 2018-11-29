// File: project4.h
// Created by: Andrew Audrain

// Header file to be used with oss.c and user.c

#ifndef PROJECT4_HEADER_FILE
#define PROJECT4_HEADER_FILE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>

/* Structures */
// Process Control Block
typedef struct {
	int pcb_Index;				// Index to in PCB associated with a specific process
	int pcb_ProcessID;			// Stores process's unique pid
	int pcb_Priority;			// Stores the priority assigned by OSS upon creation
	unsigned int pcb_TotalCPUTimeUsed[2]	// Running counter of time when process was running after being scheduled
	unsigned int pcb_TotalTimeInSystem[2];	// Running counter of time when process was alive
	unsigned int pcb_TimeUsedLastBurst;	// Temporary tracker or most recent amount of time spent running
} ProcessControlBlock;

// Structure used in the message queue 
typedef struct {
	long msg_type;		// Control what process can receive the message.
	int pid;		// Store the sending process's pid.
	int processIndex;	// Store the sending process's index in the process control block and bit vector.
	bool usedFullQuantum;	// Flag to indicate if the process was able to run for its full time quantum. 
	bool terminated;	// Flag to indicate that the process was able to terminate. 
} Message;

/* Function prototypes */
// Function to handle any termination signals from either OSS or USER.
void sig_handle ( int sig_num );
  
/* Message Queue Variables */
Message message;
int messageID;
key_t messageKey = 1994;

/* Shared Memory Variables */
// Simulated clock
int shmClockID;
int *shmClock;
key_t shmClockKey = 1993;

// Process Control Block
int shmPCBID;
ProcessControlBlock *shmPCB;
key_t shmPCBKey = 1992;

#endif
