// File: oss.c
// Created by: Andrew Audrain

// Master process to simulate the OSS scheduler

#include "project4.h"

/* Structures */
// A structure to represent a queue
typedef struct {
	int front, rear, size;
	unsigned capacity;
	int *array;  
} Queue;
                   
/* Function Prototypes */
// Queue functions
Queue* createQueue ( unsigned capacity );
int isFull ( Queue* queue ); 
int isEmpty ( Queue* queue );
void enqueue ( Queue* queue, int item );
int dequeue ( Queue* queue );
int front ( Queue* queue );
int rear ( Queue* queue );

// Other functions
bool roomForProcess ( int arr[] );
int findIndex ( int arr[] );
bool timeForNewProcess ( unsigned int systemClock[], unsigned int nextProcessTimer );
void incrementClock ( unsigned int clock[] );
void cleanUpResources();

/* Global Variables */
//Constants
const int maxCurrentProcesses = 18;	// Controls how many child processes are allowed to be alive at the same time
const int maxTotalProcesses = 100; 	// Controls how many child processes are allowed to be created in total
const int killTimer = 2; 		// Controls the amount of seconds the program can be running

// Logfile Access Pointer 
FILE *fp;


/*************************************************************************************************************/
/******************************************* Start of Main Function ******************************************/
/*************************************************************************************************************/

int main ( int argc, int *argv[] ) {
	printf ( "Hello, from oss.\n" );
	
	/* General variables */
	int i, j;			// Index variables for loop control throughout the program.
	int totalProcessesCreated = 0;	// Counter variable to track how many total processes have been created.
	int ossPid = getpid();		// Hold the pid for OSS 
	pid_t pid;
	
	srand ( time ( NULL ) );	// Seed for OSS to generate random numbers when necessary.
	
	/* Output file info */
	char logName[12] = "program.log";	// Name of the the log file that will be written to throughout the life of the program.
	int numberOFLines = 0; 			// Counter to track the size of the logfile (limited to 10,000 lines).
	fp = fopen ( logName, "w+" );		// Opens file for writing. Logfile will be overwritten after each run. 
	bool keepWriting = true;		// Variable to control when writing to the file should cease based on the 
						//	number of lines.
	
	/* Signal Handling */
	// Set the alarm
	alarm ( killTimer );
	
	// Setup handling of ctrl-c or other abnormal termination signals
	if ( signal ( SIGINT, sig_handle ) == SIG_ERR ) {
		perror ( "OSS: ctrl-c signal failed." );
	}
	
	// Setup handling of alarm termination signal 
	if ( signal ( SIGALRM, sig_handle ) == SIG_ERR ) {
		perror ( "OSS: alarm signal failed." );
	}

	/* Shared Memory */
	// Creation of shared memory for simulated system clock
	if ( ( shmClockID = shmget ( shmClockKey, ( 2 * ( sizeof ( unsigned int ) ) ), IPC_CREAT | 0666 ) ) == -1 ) {
		perror ( "OSS: Failure to create shared memory space for simulated system clock." );
		return 1;
	}
	
	// Creation of shared memory for process control block
	if ( ( shmPCBID = shmget ( shmPCBKey, ( 18 * ( sizeof ( ProcessControlBlock ) ) ), IPC_CREAT | 0666 ) ) == -1 ) {
		perror ( "OSS: Failure to create shared memory space for Process Control Block." );
		return 1;
	}
	
	// Attach to and initialize shared memory for simulated system clock
	if ( ( shmClock = (unsigned int *) shmat ( shmClockID, NULL, 0 ) ) < 0 ) {
		perror ( "OSS: Failure to attach to shared memory space for simulated system clock." );
		return 1; 
	}
	shmClock[0] = 0;	// Will hold the seconds value for the simulated system clock
	shmClock[1] = 0;	// Will hold the nanoseconds value for the simulated system clock
	
	// Attach to shared memory for Process Control Block. Initialize the index value of each portion of the block.
	if ( ( shmPCB = (ProcessControlBlock *) shmat ( shmPCBID, NULL, 0 ) ) < 0 ) {
		perror ( "OSS: Failure to attach to shared memory space for Process Control Block." );
		return 1;
	}
	for ( i = 0; i < maxCurrentProcesses; ++i ) {
		shmPCB[i].pcb_Index = i;
	}
	
	
	/* Message Queue */
	if ( ( messageID = msgget ( messageKey, IPC_CREAT | 0666 ) ) == -1 ) {
		perror ( "OSS: Failure to create the message queue." );
		return 1; 
	}
	
	/* Main Loop Variables and Preparation */
	// Setup of bit vector. Bit vector size determined by value of maxCurrentProcesses. Each index
	//	will be set to 0 by default. Once a process is created, OSS will set the flag of an index
	//	to the pid of the new process. That index will then be associated with that process until 
	//	the process terminates. Upon, process termination, OSS will reset that flag to 0 allowing
	//	a new process to be created.
	int bitVector[maxCurrentProcesses];
	for ( i = 0; i < maxCurrentProcesses; ++i ) {
		bitVector[i] = 0;
	}
	
	// Set up timer to determine when new child processes should be created. Set at 0 by default so that 
	//	a child process is created immediately. Value will then be increment by some random amount to 
	//	indicate the next time after which a new child process should be created (if the bit vector allows).
	unsigned int nextProcessTimer = 0;
	int rngTimer; 
	
	// Set up of two round robin queues. As their names imply, one queue will be for high priority processes
	//	and another will be for low priority processes. Each will be the same size to account for bad RNG
	//	with determing process priority. Size is determined by maxCurrentProcesses since there will never 
	//	be more than that many processes alive at one time. 
	Queue* highPriorityQueue = createQueue ( maxCurrentProcesses );
	Queue* lowPriorityQueue = createQueue ( maxCurrentProcesses );
	
	// Other random variables that are only used in the main loop 
	int processPriority;		// Will store the 0 or 1 (RNG) that will be assigned to each created process.
	int rngPriority;		// Will stored the randomly generated number to control 
	int tempBitVectorIndex = 0;	// Will store the current open index in the bit vector to be assigned to a new process.
	bool createProcess;		// Flag to control when the process creation logic is entered. 
	int randOverhead;		
	
	/****** Main Loop ******/
	// Loop will run until the maxTotalProcesses limit has been reached. 
	while ( totalProcessesCreated < maxTotalProcesses ) {
		
		// Check to see if the logfile has reached its line limit. If so, set the flag to false so that no 
		//	more file writes occur. 
		if ( numberOfLines >= 10000 ) {
			keepWriting = false;
		}
		
		// Set the createProcess flag to false as the default each run through the main loop. 
		createProcess = false;
		
		// Check to see if the simulated system clock has passed the time for the next process to be created and
		//	if there is room for a new process at this time. If both are true, change createProcess flag to 
		//	true. Then set a new time for the next process to be created. 
		if ( timeForNewProcess ( shmClock, nextProcessTimer ) && roomForProcess ( bitVector ) ) {
			createProcess = true;
			rngTimer = ( rand() % ( 2 - 0 + `1 ) ) + 0; 
			nextProcessTimer = rngTimer; 
		}
		
		/* Process Creation */
		// If the createProcess flag is set to true, OSS enters these branches first to create the new process
		//	before it goes on to schedule anything. 
		if ( createProcess ) {
			tempBitVectorIndex = findIndex ( bitVector );
			pid = fork();
			
			// Check for failure to fork child process.
			if ( pid < 0 ) {
				perror ( "OSS: Failure to fork child process." );
				kill ( getpid(), SIGINT );
			}
			
			// In the child process...
			if ( pid == 0 ) {
				// Store child's pid in the associated index of the bit vector
				bitVector[tempBitVectorIndex] = getpid();
				
				// To pass the index to the child process with exec, must first convert to string. 
				char intBuffer[3];
				sprintf ( intBuffer, "%d", tempBitVectorIndex );
				
				execl ( "./user", "user", intBuffer, NULL );
			} // End of child process logic
			
			// In the parent process...
			// Set the priority for newly created process.
			rngPriority = ( rand() % ( 100 - 1 + 1 ) ) + 1;
			if ( rngPriority >= 1 || rngPriority < 10 ) {
				processPriority = 1;	// High priority
			} else {
				processPriority = 0; 	// Low priority
			}
			
			// Fill in process control block info for child process to see. 
			shmPCB[tempBitVectorIndex].pcb_ProcessID = pid; 
			shmPCB[tempBitVectorIndex].pcb_Priority = processPriority;
			shmPCB[tempBitVectorIndex].pcb_TotalCPUTimeUsed[0] = 0;
			shmPCB[tempBitVectorIndex].pcb_TotalCPUTimeUsed[1] = 0;
			shmPCB[tempBitVectorIndex].pcb_TotalTimeInSystem[0] = 0;
			shmPCB[tempBitVectorIndex].pcb_TotalTimeInSystem[1] = 0;
			shmPCB[tempBitVectorIndex].pcb_TimeUsedLastBurst = 0;
			shmPCB[tempBitVectorIndex].pcb_Terminated = false;
			
			// Put the child process's pid the appropriate queue.
			if ( processPriority == 0 ) {
				if ( keepWriting ) {
					fprintf ( fp, "OSS: Generating process with PID %d (Low priority) and putting it in queue 0 at time %d:%d.\n", 
						 pid, shmClock[0], shmClock[1] );
					numberOfLines++;
				}
				enqueue ( lowPriorityQueue, pid );
			}
			if ( processPriority == 1 ) {
				if ( keepWriting ) {
					fprintf ( fp, "OSS: Generating process with PID %d (High priority) and putting it in queue 1 at time %d:%d.\n", 
					 pid, shmClock[0], shmClock[1] );
					numberOfLines++;
				}
				enqueue ( highPriorityQueue, pid ;
			}
			
			totalProcessCreated++;
		} // End of Create Process Logic
		
		/* Scheduling */
					 
		// 1. Check high priority queue. 
		// 	- if empty, check low priority queue.
		//	- if nonempty, dequeue and get pid. 
		//	- send message to that process to show it has been dispatched. 
		//	- wait for response from user process to show it has finished. 
		//	- if user terminated, update stat variables. Toggle that bit back to unoccupied. 
		//	- if user is still running, update stat variables. Put back into queue.
		//	- increment shmClock[1] by the LastUsedBurstTime
		// 2. Check low priority queue.
		//	- same as above
					 
		// Increment clock 
		randOverhead = ( rand() % ( 1000 - 0 + 1 ) ) + 0;
		shmClock[0]++;
		shmClock[1] += randOverhead;
		shmClock[0] += shmClock[1] / 1000000000;
		shmClock[1] = shmClock[1] % 1000000000;
		
	} // End of Main Loop
	
	/* Detach from and delete shared memory segments. Delete message queue. Close the outfile. */
	cleanUpResources();
	
	return 0;
}

/***************************************************************************************************************/
/******************************************* End of Main Function **********************************************/
/***************************************************************************************************************/


/* Function Definitions */

// Function to scan bit vector array to see if there is room for a new process to be added. Returns true if it 
//	finds a 0 in the array. Returns false if there are no 0's in the array. 
bool roomForProcess ( int arr[] ) {
	bool foundRoom = false;
	int index; 
	int size = sizeof ( arr );
	for ( index = 0; index < size; ++ index ) {
		if ( arr[index] == 0 ) {
			foundRoom = true;
			break;
		}
	}
	
	return foundRoom; 
}

// Function to compare the shared memory clock with the clock indicating when a new process should be created. 
//	Returns true if system clock has reached or passed the indicated time by the new process clock. Returns
//	false otherwise. 
bool timeForNewProcess ( unsigned int systemClock[], unsigned int nextProcessTimer ) {
	if ( systemClock[0] >= nextProcessTimer ) {
		return true;
	}
	else
		return false;
}

// Function to return the first available index in the bit vector.
int findIndex ( int arr[] ) {
	int index, i, size;
	size = sizeof ( arr );
	for ( i = 0; i < size; ++i ) {
		if ( arr[i] != 0 ) {
			index = i;
			break;
		}
	}
	
	return index;
}


// Function to terminate all shared memory and message queue up completion or to work with signal handling
void cleanUpResources() {
	// Close the file
	fclose ( fp );
	
	// Detach from shared memory
	shmdt ( shmClock );
	shmdt ( shmPCB );

	// Destroy shared memory
	shmctl ( shmClockID, IPC_RMID, NULL );
	shmctl ( shmPCBID, IPC_RMID, NULL );
	
	// Destroy message queue
	msgctl ( messageID, IPC_RMID, NULL );
}

// Function for signal handling.
// Handles ctrl-c from keyboard or eclipsing 2 real life seconds in run-time.
void sig_handle ( int sig_num ) {
	if ( sig_num == SIGINT || sig_num == SIGALRM ) {
		printf ( "Signal to terminate was received.\n" );
		cleanUpResources();
		kill ( 0, SIGKILL );
		wait ( NULL );
		exit ( 0 );
	}
}

// Function that increments the clock by some amount of time at different points. 
// Also makes sure that nanoseconds are converted to seconds when appropriate.
void incrementClock ( unsigned int clock[] ) {
	int processingTime = 10000; // Can be changed to adjust how much the clock is incremented.
	clock[1] += processingTime;

	clock[0] += shmClock[1] / 1000000000;
	clock[1] = shmClock[1] % 1000000000;
}

// Note: Queue code is gotten from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/

// Function to create a queue of given capacity.
// It initializes size of queue as 0.
Queue* createQueue ( unsigned capacity ) {
	Queue* queue = (Queue*) malloc ( sizeof ( Queue ) );
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;	// This is important, see the enqueue
	queue->array = (int*) malloc ( queue->capacity * sizeof ( int ) );

	return queue;
}

// Queue is full when size becomes equal to the capacity
int isFull ( Queue* queue ) {
	return ( queue->size == queue->capacity );
}

// Queue is empty when size is 0
int isEmpty ( Queue* queue ) { 
	return ( queue->size == 0 );
}

// Function to add an item to the queue.
// It changes rear and size.
void enqueue ( Queue* queue, int item ) {
	if ( isFull ( queue ) ) 
		return;
	
	queue->rear = ( queue->rear + 1 ) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
}

// Function to remove an item from queue.
// It changes front and size.
int dequeue ( Queue* queue ) {
	if ( isEmpty ( queue ) )
		return INT_MIN;

	int item = queue->array[queue->front];
	queue->front = ( queue->front + 1 ) % queue->capacity;
	queue->size = queue->size - 1;

	return item;
}

// Function to get front of queue.
int front ( Queue* queue ) {
	if ( isEmpty ( queue ) )
		return INT_MIN;

	return queue->array[queue->front];
}

// Function to get rear of queue.
int rear ( Queue* queue ) {
	if ( isEmpty ( queue ) )
		return INT_MIN;

	return queue->array[queue->rear];
}
