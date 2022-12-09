// Struct for a Process

#include "headers.h"

typedef struct Processes {
    pid_t mPid; //The pid of the process after the scheduler executes it
    
    // Read data from file input
    unsigned int Id; // ID entered in the file
    unsigned int ArrivalTime;
    unsigned int Priority; //We assume that a negative priority is not allowed
    unsigned int Runtime;

    // Calculated data
    unsigned int RemainTime;
    unsigned int WaitTime;
    unsigned int LastStop; //The last time at which this process stopped

} Process;


void PrintProcess(Process *pProcess) {  //for testing
    printf("ID = %d, ", pProcess->Id);
    printf("Arrival = %d, ", pProcess->ArrivalTime);
    printf("Runtime = %d, ", pProcess->Runtime);
    printf("Priority = %d, ", pProcess->Priority);
    printf("Remaining time = %d, ", pProcess->RemainTime);
    printf("Waiting time = %d\n", pProcess->WaitTime);
}
