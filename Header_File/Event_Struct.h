/*
==============================
|| Created By YAHYA Mohamed ||
==============================
*/

#ifndef OS_STARTER_CODE_EVENT_STRUCT_H
#define OS_STARTER_CODE_EVENT_STRUCT_H

#include "Process_Struct.h"

enum Event_Type {
    START,
    STOP,
    RESUMED,
    FINISH
};

typedef struct Event_Struct {
    Process *pProcess;
    enum Event_Type Type;
    unsigned int Time_Step;
    unsigned int Current_Remaining_Time;
    unsigned int Current_Wait_Time;
    unsigned int Turn_Around_Time;
    double Weight_Turn_Around_Time;
} Event;


//print an event using the same output file format
void PrintEvent_Console(const Event *pEvent) 
{ 
    printf("At time %d ", pEvent->Time_Step);
    printf("process %d ", pEvent->pProcess->Id);
    switch (pEvent->Type) {
        case START:
            printf("started ");
            break;
        case STOP:
            printf("stopped ");
            break;
        case RESUMED:
            printf("resumed ");
            break;
        case FINISH:
            printf("finished ");
            break;
        default:
            printf("error ");
            break;
    }
    printf("arr %d total %d ", pEvent->pProcess->ArrivalTime, pEvent->pProcess->Runtime);
    printf("remain %d wait %d", pEvent->Current_Remaining_Time, pEvent->Current_Wait_Time);
    if (pEvent->Type == FINISH)
        printf(" TA %d WTA %.2f", pEvent->Turn_Around_Time, pEvent->Weight_Turn_Around_Time);
    printf("\n");
}

void PrintEvent_File(const Event *pEvent, FILE *pFile) 
{ 
    fprintf(pFile, "At time %d ", pEvent->Time_Step);
    fprintf(pFile, "process %d ", pEvent->pProcess->Id);
    switch (pEvent->Type) {
        case START:
            fprintf(pFile, "started ");
            break;
        case STOP:
            fprintf(pFile, "stopped ");
            break;
        case RESUMED:
            fprintf(pFile, "resumed ");
            break;
        case FINISH:
            fprintf(pFile, "finished ");
            break;
        default:
            fprintf(pFile, "error ");
            break;
    }
    fprintf(pFile, "arr %d total %d ", pEvent->pProcess->ArrivalTime, pEvent->pProcess->Runtime);
    fprintf(pFile, "remain %d wait %d", pEvent->Current_Remaining_Time, pEvent->Current_Wait_Time);
    if (pEvent->Type == FINISH)
        fprintf(pFile, " TA %d WTA %.2f", pEvent->Turn_Around_Time, pEvent->Weight_Turn_Around_Time);
    fprintf(pFile, "\n");
}

#endif //OS_STARTER_CODE_EVENT_STRUCT_H
