#include "Header_File/headers.h"
#include "Header_File/Process_Queue.h"
#include "Header_File/Message_Buffer.h"
#include <string.h>

void clearResources(int);

void ReadFile();

void InitIPC();

void GetUserChoice(int*);

void ExecuteClock();

void ExecuteScheduler(int*);

void SendProcess(Process *);


int* User_decision;
pid_t Clock_Pid = 0;
pid_t Scheduler_Pid = 0;
queue Waiting_ProcessQueue;
int Generated_MsgQueueId = 0;
unsigned int Number_Of_Process = 0;  // Number of proccess readed from input file  

int main(int argc, char * argv[])
{
    //initialize the process queue
    Waiting_ProcessQueue = NewProcQueue();

    signal(SIGINT, clearResources);
    // Initialization
    // 1. Read the input files.
    ReadFile();

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    User_decision = (int*) malloc(2 * sizeof(int));    // [0] Chosen Algorithm || [1] Quanta for RR
    GetUserChoice(User_decision); //1 = HPF, 2 = SRTN, 3 = SJF, 4 = RR

    //3. initialize the IPC
    InitIPC();

    // 4. Create the scheduler and clock processes.
    ExecuteClock();
    ExecuteScheduler(User_decision);

    // 5. Use this function after creating the clock process to initialize clock
    initClk();

    
    // Generation Main Loop

    //keep looping untill the next process has arrival time equal to the current time
    while (!ProcQueueEmpty(Waiting_ProcessQueue)) {
        
        //get current time
        int current_time = getClk();

        // Pointer to the next process
        Process *pNextProcess;

        //peek the processes queue
        ProcPeek(Waiting_ProcessQueue, &pNextProcess);

        if (pNextProcess->ArrivalTime == current_time)
        {
            SendProcess(pNextProcess); //send this process to the scheduler
            ProcDequeue(Waiting_ProcessQueue, &pNextProcess); //dequeue this process from the processes queue
            kill(Scheduler_Pid, SIGUSR1); //send SIGUSR1 to the scheduler
            free(pNextProcess); //free memory allocated by this process
        }
        usleep(900000); //sleep 0.9 sec
    }

    // Clear all resources || send 0 = normal exit || other = interrupt
    clearResources(0);

}

//Clears all resources in case of interruption
void clearResources(int signum) 
{
    // 1) Interrupt all processes in case interrupt signal was sent
    if (signum == SIGINT) {
        //Interrupt forked processes
        printf("Process_Gen: Sending interrupt to scheduler\n");
        if (Scheduler_Pid)
            kill(Scheduler_Pid, SIGINT);
        
        printf("Process_Gen: Sending interrupt to clock\n");
        destroyClk(false);
        kill(Clock_Pid, SIGINT);
        
        //do not continue before the two are done
        wait(NULL);     // for scheduler
        wait(NULL);     // for clock
    } 
    else    // Wait until Scheduler finish 
    { 
        printf("Process_Gen: Waiting for scheduler to finish its job...\n");
        waitpid(Scheduler_Pid, NULL, 0); //wait until scheduler exits
        
        printf("Process_Gen: Scheduler exit signal received\n");
        
        printf("Process_Gen: Sending interrupt to clock\n");
        destroyClk(false);
        kill(Clock_Pid, SIGINT);
        
        //do not continue before clock is done
        wait(NULL);
    }

    // 2) Clear User decision array
    free (User_decision);
    
    // 3) Clear IPC resources
    if (Generated_MsgQueueId != 0)
    {
        printf("Process_Gen: Cleaning IPC resources...\n");
        if (msgctl(Generated_MsgQueueId, IPC_RMID, NULL) == -1)
            perror("Process_Gen: Error");
        else
            printf("Process_Gen: IPC cleaned successfully!\n");
    }

    // 4) Clear queue 
    printf("Process_Gen: Cleaning processes queue...\n");
    Process *pTemp;
    while (ProcDequeue(Waiting_ProcessQueue, &pTemp)) {
        free(pTemp);
    }
    printf("Process_Gen: Process queue cleaned successfully!\n");


    printf("Process_Gen: Clean!\n");
    exit(EXIT_SUCCESS);

}


// Read the input files.
void ReadFile()
{

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
        Number_Of_Process++;
        ProcEnqueue(Waiting_ProcessQueue, pProcess);

        //PrintProcess(pProcess);       // For Testing
    }

    double runtime_avg = (double) runtime_sum / Number_Of_Process;
    printf("Process_Gen: Releasing file resources...\n");
    fclose(pFile);
    if (pLine)
        free(pLine);
    printf("Process_Gen: Input file done successfully!\n");
    printf("\nProcess_Gen: Total runtime S: %dsec., M: %.2fmin., H: %.2fhour\n", runtime_sum, runtime_sum / 60.0, runtime_sum / (60.0 * 60.0));
    printf("Process_Gen: Average runtime = %.2fsec.\n", runtime_avg);
}

// Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
void GetUserChoice(int* User_Choice)
{
    printf("\nProcess_Gen: Choose scheduling algorithm:\n");
    printf("1. Preemtive Highest Priority First (PHPF).\n");
    printf("2. Shortest Remaining Time Next (SRTN).\n");
    printf("3. Short Job First (SJF).\n");
    printf("4. Round Robin (RR).\n");
    printf("Enter Your Decision: ");
    scanf("%d", &User_Choice[0]);

    bool state = true;
    while (state == true)
    {
        if (User_Choice[0] < 1 || User_Choice[0] > 4)
        {
            printf("Enter Your Decision: ");
            scanf("%d", &User_Choice[0]);
        }
        else
            state = false;
    }

    // get quanta
    if (User_Choice[0] == 4) {
        printf("Enter your quanta for Round Robin: ");
        scanf("%d", &User_Choice[1]);
    }

    switch (User_Choice[0])
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
            printf("Your Decision: Round Robin (RR) with quanta = %d\n", User_Choice[1]);
            break;
        default:
            printf("Your Decision: %d\n", User_Choice[0]);
    }
}

// Initialize the IPC
void InitIPC()
{
    key_t key = ftok(Ftok_File, Ftok_Key);
    Generated_MsgQueueId = msgget(key, 0666 | IPC_CREAT);
    if (Generated_MsgQueueId == -1) {
        perror("Process_Gen: IPC init failed");
        raise(SIGINT);
    }
    printf("Process_Gen: IPC ready!\n");
}


void ExecuteClock()
{

    Clock_Pid = fork();
    while (Clock_Pid == -1) {
        perror("Process_Gen: Error forking clock");
        printf("Process_Gen: Wait! Trying again...\n");
        Clock_Pid = fork();
    }
    if (Clock_Pid == 0) {
        printf("Process_Gen: Clock forked successfully!\n");
        printf("Process_Gen: Executing clock...\n");
        char *argv[] = {"clk.out", NULL};       // clk must be compiled in the terminal first
        execv("clk.out", argv);
        perror("Process_Gen: Clock execution failed");
        exit(EXIT_FAILURE);
    }

}

void ExecuteScheduler(int* User_Choice)
{
    Scheduler_Pid = fork();
    while (Scheduler_Pid == -1) {
        perror("Process_Gen: Error forking scheduler");
        printf("Process_Gen: Wait! Trying again...\n");
        Scheduler_Pid = fork();
    }
    if (Scheduler_Pid == 0) {
        printf("Process_Gen: Scheduler forked successfully!\n");
        printf("Process_Gen: Executing scheduler...\n");

        char buffer[10];                //buffer to convert from string to int
        sprintf(buffer, "%d", Number_Of_Process);
        char *argv[4];                  // last element must be NULL
        argv[1] = buffer;               // for knowing the number of process that will be sent
        argv[2] = NULL;                 // for any algoritm except RR {Last element must be NULL}    
        argv[3] = NULL;                 // for RR {as argv[2] = Quanta}
               
        switch (User_Choice[0]) {       // Algorithm must be compiled in the terminal first
            case 1:
                argv[0] = "PHPF.out";
                execv("PHPF.out", argv);
                break;
            case 2:
                argv[0] = "SRTN.out";
                execv("SRTN.out", argv);
                break;
            case 3:
                argv[0] = "SJF.out";
                execv("SJF.out", argv);
                break;
            case 4:
                sprintf(buffer, "%d", User_Choice[1]);
                argv[2] = buffer;
                argv[0] = "RR.out";
                execv("RR.out", argv);
                break;
            default:
                break;
        }
        perror("Process_Gen: Scheduler execution failed");
        exit(EXIT_FAILURE);
    }
}


// Sending the process to the scheduler
void SendProcess(Process *pProcess)
{
    Message msg;
    msg.mType = 7; //Any value for mType
    msg.mProcess = *pProcess;
    printf("Process_Gen: Sending process with id %d to scheduler...\n", msg.mProcess.Id);
    
    int State = msgsnd(Generated_MsgQueueId, &msg, sizeof(msg.mProcess), !IPC_NOWAIT);

    if (State== -1) 
    {
        perror("Process_Gen: Error while sending process");
    } 
    else 
    {
        printf("Process_Gen: Process with id %d sent successfully!\n", msg.mProcess.Id);
    }
}
