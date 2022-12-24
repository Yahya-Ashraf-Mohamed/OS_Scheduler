#ifndef OS_STARTER_CODE_SCHEDULER_H
#define OS_STARTER_CODE_SCHEDULER_H


#include "headers.h"
#include "Events_Queue.h"

Process *pCurrent_Running_Process = NULL; //pointer to the current running process
EventQueue = NewEventQueue(); //init event queue
int Received_MsgQueue_Id = 0; //msg queue id to receive processes from process_generator

// in each process AddEvent(#state of the event {stopped, resumed, finished, started});


// Initialize the IPC and Connect it the process_generator
void InitIPC()
{
    key_t key = ftok(Ftok_File, Ftok_Key); //same parameters used in process_generator
    Received_MsgQueue_Id = msgget(key, 0);
    if (Received_MsgQueue_Id == -1) {
        perror("RoundRobin: Round Robin IPC init failed!");
        raise(SIGINT);
    }
    printf("RoundRobin: Round Robin IPC ready...\n");
}


void AddEvent(enum Event_Type Etype)
{
    Event *pCurrentEvent = malloc(sizeof(Event));
    while (!pCurrentEvent) {
        perror("RoundRobin: Malloc failed");
        printf("RoundRobin: Trying again");
        pCurrentEvent = malloc(sizeof(Event));
    }

    pCurrentEvent->Time_Step = getClk();
    if (Etype == FINISH) {
        pCurrentEvent->Turn_Around_Time = getClk() - pCurrentProcess->ArrivalTime;
        pCurrentEvent->Weight_Turn_Around_Time = (double) pCurrentEvent->Turn_Around_Time / pCurrentProcess->Runtime;
    }
    pCurrentEvent->pProcess = pCurrentProcess;
    pCurrentEvent->Current_Wait_Time = pCurrentProcess->WaitTime;
    pCurrentEvent->Type = Etype;
    pCurrentEvent->Current_Remaining_Time = pCurrentProcess->RemainTime;
    EventQueueEnqueue(Event_Queue, pCurrentEvent);
}


void Log_AllEvents(unsigned int start_time, unsigned int end_time)
{
    unsigned int runtime_sum = 0,
                 waiting_sum = 0, 
                 count = 0;
    
    double wta_sum = 0,
           wta_squared_sum = 0;

    FILE *pFile = fopen("Events_Log.txt", "w");
    Event *pNextEvent = NULL;

    // Loop until event queue is empty
    while (EventQueueDequeue(Event_Queue, &pNextEvent)) 
    { 
        PrintEvent_Console(pNextEvent);
        PrintEvent_File(pNextEvent, pFile);
        if (pNextEvent->Type == FINISH) {
            runtime_sum += pNextEvent->pProcess->Runtime;
            waiting_sum += pNextEvent->Current_Wait_Time;
            count++;
            wta_sum += pNextEvent->Weight_Turn_Around_Time;
            wta_squared_sum += pNextEvent->Weight_Turn_Around_Time * pNextEvent->Weight_Turn_Around_Time;
            free(pNextEvent->pProcess);
        }
        free(pNextEvent); //free memory allocated by the event
    }
    fclose(pFile);
 
    double cpu_utilization = runtime_sum * 100.0 / (end_time - start_time);
    double avg_wta = wta_sum / count;
    double avg_waiting = (double) waiting_sum / count;

    pFile = fopen("Stats.txt", "w");
    printf("\nCPU utilization = %.2f\n", cpu_utilization);
    printf("Avg WTA = %.2f\n", avg_wta);
    printf("Avg Waiting = %.2f\n", avg_waiting);

    fprintf(pFile, "Avg WTA = %.2f\n", avg_wta);
    fprintf(pFile, "Avg Waiting = %.2f\n", avg_waiting);
    fprintf(pFile, "\nCPU utilization = %.2f\n", cpu_utilization);
}

#endif