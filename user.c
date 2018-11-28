// File: user.c
// Created by: Andrew Audrain 

// User process which is generated and scheduled by OSS
#include "project4.h"

int main ( int argc, char *argv[] ) {
	/* General Variables */
	int i, j;				// Loop index variables.
	int myPid = getpid();			// Store process ID.
	int ossPid = getppid();			// Store parent process ID.
	int tableIndex = atoi ( argv[1] );	// Store process control block index passed from OSS. 
	
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
	
	// Attach to shared memory for Process Control Block.
	if ( ( shmPCB = (ProcessControlBlock *) shmat ( shmPCBID, NULL, 0 ) ) < 0 ) {
		perror ( "USER: Failure to attach to shared memory space for Process Control Block." );
		return 1;
	}
	
	/* Message Queue */
	// Access message queue.
	if ( ( messageID = msgget ( messageKey, IPC_CREAT | 0666 ) ) == -1 ) {
		perror ( "USER: Failure to create the message queue." );
		return 1; 
	}
	
	
	return 0;
}

void sig_handle ( int sig_num ) {
	if ( sig_num == SIGINT ) {
		printf ( "%d: Signal to terminate process was received.\n", getpid() );
		exit ( 0 );
	}
}
