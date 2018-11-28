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
	pid_t childPid;			// Hold the pid for child process during process creation phase.
	bool keepWriting = true;	// Variable to control when writing to the file should cease based on the 
					//	number of lines.
	srand ( time ( NULL ) );	// Seed for OSS to generate random numbers when necessary.
	
	/* Output file info */
	char logName[12] = "program.log";	// Name of the the log file that will be written to throughout the life of the program.
	int numberOFLines = 0; 			// Counter to track the size of the logfile (limited to 10,000 lines).
	fp = fopen ( logName, "w+" );		// Opens file for writing. Logfile will be overwritten after each run. 
	fprintf ( fp, "Testing...\n" );
	
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
	
	// Attach to shared memory for Process Control Block 
	if ( ( shmPCB = (ProcessControlBlock *) shmat ( shmPCBID, NULL, 0 ) ) < 0 ) {
		perror ( "OSS: Failure to attach to shared memory space for Process Control Block." );
		return 1;
	}
	
	/* Message Queue */
	if ( ( messageID = msgget ( messageKey, IPC_CREAT | 0666 ) ) == -1 ) {
		perror ( "OSS: Failure to create the message queue." );
		return 1; 
	}
	
	/* Main Loop Variables and Preparation */
	// Setup of bit vector. Bit vector size determined by value of maxCurrentProcesses. Each index
	//	will be set to 0 by default. Once a process is created, OSS will set the flag of an index
	//	to 1. That index will then be associated with that process until the process terminates. 
	//	Upon, process termination, OSS will reset that flag to 0 allowing a new process to be created.
	int bitVector[maxCurrentProcesses];
	for ( i = 0; i < maxCurrentProcesses; ++i ) {
		bitVector[i] = 0;
	}
	
	// Set up clock to determine when new child processes should be created. Set at 0 by default so that 
	//	a child process is created immediately. Value will then be increment by some random amount to 
	//	indicate the next time after which a new child process should be created (if the bit vector allows).
	unsigned int nextProcessTimer[2]; 
	nextProcessTimer[0] = 0;
	nextProcessTimer[1] = 0;
	
	// Set up of two round robin queues. As their names imply, one queue will be for high priority processes
	//	and another will be for low priority processes. Each will be the same size to account for bad RNG
	//	with determing process priority. Size is determined by maxCurrentProcesses since there will never 
	//	be more than that many processes alive at one time. 
	Queue* highPriorityQueue = createQueue ( maxCurrentProcesses );
	Queue* lowPriorityQueue = createQueue ( maxCurrentProcesses );
	
	
	
	/* Detach from and delete shared memory segments. Delete message queue. Close the outfile. */
	cleanUpResources();
	
	return 0;
}

/***************************************************************************************************************/
/******************************************* End of Main Function **********************************************/
/***************************************************************************************************************/


/* Function Definitions */

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
