// File: user.c
// Created by: Andrew Audrain 

// User process which is generated and scheduled by OSS
#include "project4.h"

const unsigned int baseQuantum = 50000;

int main ( int argc, char *argv[] ) {
	/* General Variables */
	int i, j;				// Loop index variables.
	int myPid = getpid();			// Store process ID.
	long ossPid = getppid();		// Store parent process ID.
	int tableIndex = atoi ( argv[1] );	// Store process control block index passed from OSS. 
	int priority;				// Store the priority that is in the process control block.
	unsigned int timeCreated[2];		// Store the the process entered the system. 
	unsigned int quantum;			// Time quantum for whenever process is dispatched. 
	int timeSlice; 				// Will determine how much of the quantum the process uses each dispatch
	int randTerminate;			// Will randomly determine if the process terminated. 
	unsigned timeSliceUsed; 
	
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
		// Wait until a message is received from OSS which will indicate the process was dispatched.
		// Blocked until a message is received. 
		msgrcv ( messageID, &message, sizeof ( message ), myPid, 0 ); 
		
		// Determine if process will terminate. Process must have accumulated at least 50 milliseconds of
		//	total CPU time. 
		if ( shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] >= 500000 ) {
			randTerminate = ( rand() % ( 100 - 0 + 1) ) + 0;
			if ( randTerminate >= 0 && randTerminate < 25 ) {
				// Process has decided it is able to terminate. 
				// Determine how much of time slice was used during this last run. Randomly generate a 1 or 0.
				//	0 indicates whole time slice was used. 1 indicates just a portion was used. 
				timeSlice = ( rand() % ( 1 - 0 + 1 ) ) + 0;
				if ( timeSlice == 0 ) {
					shmPCB[tableIndex].pcb_TimeUsedLastBurst = quantum;
					shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] += quantum; 
					shmPCB[tableIndex].pcb_TotalCPUTimeUsed[0] += shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] / 1000000000;
					shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] = shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] % 1000000000;
				}
				if ( timeSlice == 1 ) {
					timeSliceUsed = ( rand() % ( quantum - 0 + 1 ) ) + 0;
					shmPCB[tableIndex].pcb_TimeUsedLastBurst = timeSliceUsed;
					shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] += timeSliceUsed; 
					shmPCB[tableIndex].pcb_TotalCPUTimeUsed[0] += shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] / 1000000000;
					shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] = shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] % 1000000000;
				}				
				
				// Update total time in system by subtracting the time it entered the system from the current
				//	time in the simulated system clock. 
				shmPCB[tableIndex].pcb_totalTimeInSystem[0] = shmClock[0] - timeCreated[0];
				shmPCB[tableIndex].pcb_totalTimeInSystem[1] = shmClock[1] - timeCreated[1];

				// Send message to OSS indicating that the process has terminated.
				message.msg_type = ossPid;		
				message.pid = myPid;		
				message.processIndex = tableIndex;	
				message.terminated = true;	
				
				if ( msgsnd ( messageID, &message, sizeof ( message ), 0 ) == -1 ) {
					perror ( "USER: Failure to send message." );
				}
				break;
			}
		} // End of terminate branch
		
		// If process isn't terminating, determine how much of time quantum process will use. 
		timeSlice = ( rand() % ( 1 - 0 + 1 ) ) + 0;
		if ( timeSlice == 0 ) {
			shmPCB[tableIndex].pcb_TimeUsedLastBurst = quantum;
			shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] += quantum; 
			shmPCB[tableIndex].pcb_TotalCPUTimeUsed[0] += shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] / 1000000000;
			shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] = shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] % 1000000000;
		}
		if ( timeSlice == 1 ) {
			timeSliceUsed = ( rand() % ( quantum - 0 + 1 ) ) + 0;
			shmPCB[tableIndex].pcb_TimeUsedLastBurst = timeSliceUsed;
			shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] += timeSliceUsed; 
			shmPCB[tableIndex].pcb_TotalCPUTimeUsed[0] += shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] / 1000000000;
			shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] = shmPCB[tableIndex].pcb_TotalCPUTimeUsed[1] % 1000000000;
		}

		// Send message to OSS indicating that the process has finished reached its quantum or slice of quantum.
		message.msg_type = ossPid;		
		message.pid = myPid;		
		message.processIndex = tableIndex;	
		message.terminated = false;	

		if ( msgsnd ( messageID, &message, sizeof ( message ), 0 ) == -1 ) {
			perror ( "USER: Failure to send message." );
		}
	} // End of main loop
	
	return 0;
}

void sig_handle ( int sig_num ) {
	if ( sig_num == SIGINT ) {
		printf ( "%d: Signal to terminate process was received.\n", getpid() );
		exit ( 0 );
	}
}
