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

// Logfile Information 
FILE *fp;
char filename[12] = "program.log";	// Name of the the log file that will be written to throughout the life of the program
int numberOFLines; 			// Counter to track the size of the logfile (limited to 10,000 lines)


/*************************************************************************************************************/
/******************************************* Start of Main Function ******************************************/
/*************************************************************************************************************/

int main ( int argc, int *argv[] ) {
	printf ( "Hello, from oss.\n" );
  
	return 0;
}

/***************************************************************************************************************/
/******************************************* End of Main Function **********************************************/
/***************************************************************************************************************/


/* Function Definitions */

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
