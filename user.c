// File: user.c
// Created by: Andrew Audrain 

// User process which is generated and scheduled by OSS
#include "project4.h"

const int baseQuantum = 6;

int main ( int argc, char *argv[] ) {
	/* General Variables */
	int i, j;				// Loop index variables.
	int myPid = getpid();			// Store process ID.
	int ossPid = getppid();			// Store parent process ID.
	int tableIndex = atoi ( argv[1] );	// Store process control block index passed from OSS. 
	int priority;				// Store the priority that is in the process control block.
	unsigned int timeCreated[2];		// Store the the process entered the system. 
	int quantum;				// Time quantum for whenever process is dispatched. 
	int timeSlice; 				// Will determine how much of the quantum the process uses each dispatch
	int randTerminate;			// Will randomly determine if the process terminated. 
	
	/* USER-specific seed for random number generation */
	time_t childSeed;
	srand ( ( int ) time ( &childSeed ) % getpid() );

	/* Signal Handling */
	if ( signal ( SIGINT, sig_handle ) == SIG_ERR ) {
		perror ( "USER: signal failed." );
	}
	
	/* Attach to shared memory */
	// Find shared memory for simulated system clock.
	if ( ( shmClockID = shmget ( shmClockKey, ( 2 * ( sizeof ( unsigned int ) ) ), IPC_CREAT ) ) == -1 ) {
		perror ( "USER: Failure to get shared memory space for simulated system clock." );
		return 1;
	}
	
	// Find shared memory for process control block.
	if ( ( shmPCBID = shmget ( shmPCBKey, ( 18 * ( sizeof ( ProcessControlBlock ) ) ), IPC_CREAT ) ) == -1 ) {
		perror ( "USER: Failure to get shared memory space for Process Control Block." );
		return 1;
	}
	
	// Attach to shared memory for simulated system clock
	if ( ( shmClock = (unsigned int *) shmat ( shmClockID, NULL, 0 ) ) < 0 ) {
		perror ( "USER: Failure to attach to shared memory space for simulated system clock." );
		return 1; 
	}
	
	// Set the time the process was created after attaching to shared memory clock.
	timeCreated[0] = shmClock[0];
	timeCreated[1] = shmClock[1];
	
	// Attach to shared memory for Process Control Block.
	if ( ( shmPCB = (ProcessControlBlock *) shmat ( shmPCBID, NULL, 0 ) ) < 0 ) {
		perror ( "USER: Failure to attach to shared memory space for Process Control Block." );
		return 1;
	}
	
	// Determine time quantum based off of process priorirty stored in process control block. 
	priority = shmPCB[tableIndex].pcb_Priority;
	if ( priority == 1 ) {
		quantum = baseQuantum / 2;
	} else {
		quantum = baseQuantum; 
	}
	
	/* Message Queue */
	// Access message queue.
	if ( ( messageID = msgget ( messageKey, IPC_CREAT | 0666 ) ) == -1 ) {
		perror ( "USER: Failure to create the message queue." );
		return 1; 
	}
	
	/* Main Loop */
	while ( 1 ) {
		// 1. Wait for message saying process has been dispatched.
		// 2. Determine if process will terminate.
		// 3. Determine how much of time quantum process will use. 
		// 4. Update process control block.
		// 5. Send message to OSS.
	}
	
	return 0;
}

void sig_handle ( int sig_num ) {
	if ( sig_num == SIGINT ) {
		printf ( "%d: Signal to terminate process was received.\n", getpid() );
		exit ( 0 );
	}
}
