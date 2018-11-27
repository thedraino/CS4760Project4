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
    int pcb_TotalCPUTimeUsed;
    int pcb_TotalTimeInSystem;
    int pcb_TimeUsedLastBurst;
    int pcb_Priority;
    int pcb_ProcessID;
    int pcb_Index; 
    unsigned int pcb_ArrivalTime[2];
    bool pcb_Terminated;
} ProcessControlBlock;

// Structure used in the message queue 
typedef struct {
	long msg_type;		// Control what process can receive the message.
	int pid;		    // Store the sending process's pid.
	int address;		// Store the address location the process is requesting in memory. 
	bool read;		    // Flag is set to true if memory reference is read from memory. 
	bool write;		    // Flag is set to fales if memory reference is a write to memory. 
	bool terminate;		// Default is false. Gets changed to true when child terminates. 
	bool suspended;		// Default is false. Will be changed if indicated upon receiving a message from OSS. 
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
