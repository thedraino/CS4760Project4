// File: oss.h
// Created by: Andrew Audrain

// Header file to be used with oss.c and user.c

#ifndef OSS_HEADER_FILE
#define OSS_HEADER_FILE

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
#include <sys/types.h>
#include <sys/ipc.h>

// Structures
// Process Control Block
typedef struct {
    int pcb_TotalCPUTimeUsed;
    int pcb_TotalTimeInSystem;
    int pcb_TimeUsedLastBurst;
    int pcb_Priority;
    int pcb_ProcessID;
    int pcb_Index; 
    int pcb_ArrivalTimeSec;
    unsigned int pcb_ArrivalTimeNano;
    bool pcb_Terminated;
} ProcessControlBlock;

// Queue code is gotten from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
// A structure to represent a queue
typdef struct {
    int front, rear, size;
    unsigned capacity;
    int *array;  
} Queue;
  
// Queue Protypte Functions
// Functions will be only defined in oss.c since they are not used in user.c
struct Queue* createQueue ( unsigned capacity );
int isFull ( struct Queue* queue ); 
int isEmpty ( struct Queue* queue );
void enqueue ( struct Queue* queue, int item );
int dequeue ( struct Queue* queue );
int front ( struct Queue* queue );
int rear ( struct Queue* queue );

#endif 
