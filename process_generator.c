//#include "Header_File/headers.h"
#include "Header_File/Process_Queue.h"

#include <string.h>
#include "math.h"

void clearResources(int);

void ReadFile();

queue gProcessQueue;

int main(int argc, char * argv[])
{
    //initialize the process queue
    gProcessQueue = NewProcQueue();

    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    ReadFile();

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int User_Choice;
    User_Choice = GetUserChoice(); //1 = HPF, 2 = SRTN, 3 = SJF, 4 = RR
/*
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
*/
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}








// Read the input files.

void ReadFile() {

    printf("Process_Gen: Attempting to open input file...\n");
    
    FILE *pFile;
    char *pLine = NULL;
    size_t len = 0;
    ssize_t read;
    
    pFile = fopen("processes.txt", "r");
    if (pFile == NULL) {
        perror("Process_Gen: Error in openning input file");
        exit(EXIT_FAILURE);
    }

    printf("Process_Gen: Reading input file...\n");
    
    unsigned int runtime_sum = 0;
    unsigned int runtime_squared_sum = 0;
    unsigned int count = 0;
    
    while ((read = getline(&pLine, &len, pFile)) != -1)
    {
        if (pLine[0] == '#') //skip comments in the input file
            continue;

        Process *pProcess = malloc(sizeof(Process));

        // Check for success decleration pointer to Process
        while (!pProcess) {
            perror("Process_Gen: Malloc failed");
            printf("Process_Gen: Trying again");
            pProcess = malloc(sizeof(Process));
        }

        pProcess->Id = atoi(strtok(pLine, "\t"));
        pProcess->ArrivalTime = atoi(strtok(NULL, "\t"));
        pProcess->Runtime = atoi(strtok(NULL, "\t"));
        pProcess->Priority = atoi(strtok(NULL, "\t"));
        pProcess->RemainTime = pProcess->Runtime;
        pProcess->WaitTime = 0;
        runtime_sum += pProcess->Runtime;
        runtime_squared_sum += pProcess->Runtime * pProcess->Runtime;
        count++;
        ProcEnqueue(gProcessQueue, pProcess);

        //PrintProcess(pProcess);       // For Testing
    }

    double runtime_avg = (double) runtime_sum / count;
    printf("Process_Gen: Releasing file resources...\n");
    fclose(pFile);
    if (pLine)
        free(pLine);
    printf("Process_Gen: Input file done successfully!\n");
    printf("\nProcess_Gen: Total runtime S: %dsec., M: %.2fmin., H: %.2fhour\n", runtime_sum, runtime_sum / 60.0, runtime_sum / (60.0 * 60.0));
    printf("Process_Gen: Average runtime = %.2fsec.\n", runtime_avg);
}

// Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
int GetUserChoice() {
    int User_decision;
    printf("\nProcess_Gen: Choose scheduling algorithm:\n");
    printf("1. Preemtive Highest Priority First (PHPF).\n");
    printf("2. Shortest Remaining Time Next (SRTN).\n");
    printf("3. Short Job First (SJF).\n");
    printf("4. Round Robin (RR).\n");
    printf("Enter Your Decision: ");
    scanf("%d", &User_decision);

    bool state = true;
    while (state == true)
    {
        if (User_decision < 1 || User_decision > 4)
        {
            printf("Enter Your Decision: ");
            scanf("%d", &User_decision);
        }
        else
            state = false;
    }

    int quanta = 0;
    if (User_decision == 4) {
        printf("Enter your quanta for Round Robin: ");
        scanf("%d", &quanta);
    }

    switch (User_decision)
    {
        case 1:
            printf("Your Decision: Preemtive Highest Priority First (PHPF)\n");
            break;
        case 2:
            printf("Your Decision: Shortest Remaining Time Next (SRTN)\n");
            break;
        case 3:
            printf("Your Decision: Short Job First (SJF)\n");
            break;
        case 4:
            printf("Your Decision: Round Robin (RR) with quanta = %d\n", quanta);
            break;
        default:
            printf("Your Decision: %d\n", User_decision);
    }
    
    return User_decision;
}