#ifndef OS_STARTER_CODE_SCHEDULER_H
#define OS_STARTER_CODE_SCHEDULER_H


#include "headers.h"
#include "Headers/EventsQueue.h"

Process *pCurrent_Running_Process = NULL; //pointer to the current running process
EventQueue = NewEventQueue(); //init event queue

// in each process AddEvent(#state of the event {stopped, resumed, finished, started});


void AddEvent(enum EventType Etype)
{
    Event *pCurrentEvent = malloc(sizeof(Event));
    while (!pCurrentEvent) {
        perror("RoundRobin: Malloc failed");
        printf("RoundRobin: Trying again");
        pCurrentEvent = malloc(sizeof(Event));
    }

    pCurrentEvent->Time_Step = getClk();
    if (Etype == FINISH) {
        pCurrentEvent->Turn_Around_Time = getClk() - pCurrent_Running_Process->ArrivalTime;
        pCurrentEvent->Weight_Turn_Around_Time = (double) pCurrentEvent->Turn_Around_Time / pCurrent_Running_Process->Runtime;
    }
    pCurrentEvent->pProcess = pCurrent_Running_Process;
    pCurrentEvent->Current_Wait_Time = pCurrent_Running_Process->WaitTime;
    pCurrentEvent->Type = Etype;
    pCurrentEvent->Current_Remaining_Time = pCurrent_Running_Process->RemainTime;
    EventQueueEnqueue(EventQueue, pCurrentEvent);
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
    while (EventQueueDequeue(gEventQueue, &pNextEvent)) 
    { 
        PrintEvent_Console(pNextEvent);
        OutputEvent_File(pNextEvent, pFile);
        if (pNextEvent->mType == FINISH) {
            runtime_sum += pNextEvent->mpProcess->mRuntime;
            waiting_sum += pNextEvent->mCurrentWaitTime;
            count++;
            wta_sum += pNextEvent->mWTaTime;
            wta_squared_sum += pNextEvent->mWTaTime * pNextEvent->mWTaTime;
            free(pNextEvent->mpProcess);
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
    printf("STD WTA = %.2f\n\n", std_wta);

    fprintf(pFile, "Avg WTA = %.2f\n", avg_wta);
    fprintf(pFile, "Avg Waiting = %.2f\n", avg_waiting);
    fprintf(pFile, "\nCPU utilization = %.2f\n", cpu_utilization);
}

#endif