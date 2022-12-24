#include "Header_File/headers.h"
#include "Header_File/Process_Struct.h"
#include "Header_File/Process_Heap.h"
#include "Header_File/Message_Buffer.h"
#include "Header_File/Events_Queue.h"
#include "Header_File/Process_Queue.h"
#include <math.h>


void InitIPC();
void Process_Arrival_Handler(int);
int Receive_Process();
void AddEvent(enum Event_Type Etype);
void Clean_Resources(int);
void Execute_Process();
void Alarm_Handler(int);
void Log_AllEvents(unsigned int, unsigned int);
void Process_Termination(int);
int isTie();

unsigned int Quanta;
unsigned int Number_Of_Process;  // Number of proccess readed from input file  

queue Process_Queue; //main processes queue
event_queue Event_Queue; //queue for generated events to be excuted later
int Received_MsgQueue_Id = 0; //msg queue id to receive processes from process_generator

Process *pCurrentProcess = NULL; //pointer to the current running process
heap_t *Process_Heap; //heap for processes arranged according to their arrival time

unsigned short Switch_Context_Flag = 0; //flag switch context {1 = switch, 0 = do not switch}

int main(int argc, char *argv[]) {
    Number_Of_Process = atoi(argv[1]); //get Number of proccess readed from input file
    Quanta = atoi(argv[2]); //get quanta
    printf("RoundRobin: Round_Robin Started with Quanta: %d\n", Quanta);

    // [1] Initialize Data & Queues
    initClk(); //initialize clock
    InitIPC(); // Creat and Connect message queue with the Message queue of the process_generator
    
    Event_Queue = NewEventQueue(); //init event queue
    Process_Queue = NewProcQueue(); //init processes queue
    Process_Heap = (heap_t *) calloc(1, sizeof(heap_t)); //init processes heap

    // [2] Create Signals Handlers
    signal(SIGUSR1, Process_Arrival_Handler); //handle SIGUSR1 sent by process_generator when new process is available
    signal(SIGINT, Clean_Resources); //handle SIGINT to Clean resources
    signal(SIGALRM, Alarm_Handler); //handle alarm signals received at the end of each quanta 
    signal(SIGCHLD, Process_Termination); //handle when a process finishes execution using ChildHandler

    // [3] Loop
    pause(); // Wait for the first process to arrive
    unsigned int start_time = getClk();     // Get the start time

    while (ProcDequeue(Process_Queue, &pCurrentProcess) /*|| Number_Of_Process!=0*/) // While processes queue is not empty or their is a process that will be sent
    {

        if (isTie()) // Check if their is a tie & if yes put them in the heap
        { 
            // As long as heap is not empty
            while (!HeapEmpty(Process_Heap))
            {   
                pCurrentProcess = HeapPop(Process_Heap); // Get highest priority process
                // Cretical section!
                Switch_Context_Flag = 0; //turn off switching   [LOCK]
                Execute_Process(); //execute current process
                while (!Switch_Context_Flag) //as long as this flag is set to zero keep pausing until Alarm Signal is sent
                    pause(); // To Avoid Busy Waiting
            }
            continue; //after handling all tie processes skip below and dequeue a new process from main queue
        }
        
        // Cretical section!
        Switch_Context_Flag = 0; //turn off switching   [LOCK]
        Execute_Process(); //execute current process
        while (!Switch_Context_Flag) //as long as this flag is set to zero keep pausing until Alarm Signal is sent
            pause(); // To Avoid Busy Waiting
    }
    unsigned int finish_time = getClk();    // Get the finish time
    Log_AllEvents(start_time, finish_time);
    raise(SIGINT);  // Clean & Exit   
}

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


void Process_Arrival_Handler(int signum)
{
    //keep looping as long as a process was received in the current iteration
    while (!Receive_Process());
}


int Receive_Process()
{
    Message msg;
    // receive a message or return immediately if their is no proecees
    if (msgrcv(Received_MsgQueue_Id, (void *) &msg, sizeof(msg.mProcess), 0, IPC_NOWAIT) == -1) {
        perror("RoundRobin: Error while receiving!");
        return -1;
    }

    // if message was retrieved from the message queue
    printf("RoundRobin: Received by Round Robin scheduler\n");
    Process *pProcess = malloc(sizeof(Process));    // allocate memory for the received process
    while (!pProcess) {
        perror("RoundRobin: Malloc Failed!");
        printf("RoundRobin: Trying Again...");
        pProcess = malloc(sizeof(Process));
    }
    *pProcess = msg.mProcess; //store the process received in the allocated space
    ProcEnqueue(Process_Queue, pProcess);//push the process pointer into the main processes queue
    return 0;
}


void Clean_Resources(int signum) 
{
    printf("RoundRobin: Cleaning Round Roubin scheduler resources...\n");
    
    Process *pProcess = NULL;   // deallocate before the pointer holds any process
    while (ProcDequeue(Process_Queue, &pProcess)) //while processes queue is not empty
        free(pProcess); //free memory allocated for this process

    Event *pEvent = NULL;  // deallocate before the pointer holds any event
    while (EventQueueDequeue(Event_Queue, &pEvent)) //while event queue is not empty
        free(pEvent); //free memory allocated by the event
    printf("RoundRobin: Done! Round Robin Scheduler is now clean.\n");
    exit(EXIT_SUCCESS);
}


void Alarm_Handler(int signum)
{
    pCurrentProcess->RemainTime -= Quanta; // Remaining time should have been decreased by the value of the quanta
    if (!pCurrentProcess->RemainTime) // If this process should finish END
        return;

    if (ProcQueueEmpty(Process_Queue)) // If there is no other processes are available give the current process an extra quanta
    { 
        alarm(Quanta);
        return;
    }

    if (kill(pCurrentProcess->Pid, SIGTSTP) == -1) { // Stop current process
        perror("RoundRobin: Error stopping process!");
        return;
    }
    pCurrentProcess->LastStop = getClk(); // Save stop time
    ProcEnqueue(Process_Queue, pCurrentProcess); //re-enqueue the process to the queue
    AddEvent(STOP);

    Switch_Context_Flag = 1; // flag == 1 so main loop knows it's time to switch context     [UNLOCK]
}


void Execute_Process() {
    // [Case 1] the process never ran before
    if (pCurrentProcess->Runtime == pCurrentProcess->RemainTime) 
    { 
        pCurrentProcess->Pid = fork(); // fork a new child and store its pid in the process struct
        while (pCurrentProcess->Pid == -1) // In case forking faild 
        { 
            perror("RoundRobin: Error forking process!");
            printf("RoundRobin: Trying again...\n");
            pCurrentProcess->Pid = fork();
        }
        // If I am child then execute the process
        if (!pCurrentProcess->Pid) 
        {
            char buffer[10]; //buffer to convert runtime from int to string
            sprintf(buffer, "%d", pCurrentProcess->Runtime);
            char *argv[] = {"process.out", buffer, NULL};
            execv("process.out", argv);
            perror("RoundRobin: ERROR! Process execution failed");
            exit(EXIT_FAILURE);
        }

        alarm(Quanta); // Generate an alarm to raise signal when quanta is over [Call SIGALRM]
        //initial wait time for the process (Start time - Arival time)
        pCurrentProcess->WaitTime = getClk() - pCurrentProcess->ArrivalTime;
        AddEvent(START); // Create a start event
    } 
    // [Case 2] the process was stopped and its turn to resume
    else 
    { 
        if (kill(pCurrentProcess->Pid, SIGCONT) == -1)   //continue process
        {
            printf("RoundRobin: Error resuming process %d!", pCurrentProcess->Id);
            perror(NULL);
            return;
        }
        alarm(Quanta); // Generate an alarm to raise signal when quanta is over [Call SIGALRM]
        pCurrentProcess->WaitTime += getClk() - pCurrentProcess->LastStop; //add the additional waiting time  (Current time - Last Stop time)
        AddEvent(RESUMED);
    }
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


void Process_Termination(int signum)
{
    if (!waitpid(pCurrentProcess->Pid, NULL, WNOHANG)) // if current process did not terminate. return back
        return;

    alarm(0); //cancel any currently active alarm
    pCurrentProcess->RemainTime = 0; //process finished
    AddEvent(FINISH);

    Switch_Context_Flag = 1; //set flag to 1 so main loop knows it's time to switch context as
                            // if the process Finished before quanta time Ends other process will start
}

int isTie() 
{

    // [Case 1] If processes queue is empty => nothing to peek => no tie
    Process *pTemp;
    if (!ProcPeek(Process_Queue, &pTemp)) 
        return 0;

    // [Case 2] If this process ran before so no tie exists
    if (pCurrentProcess->Runtime > pCurrentProcess->RemainTime)
        return 0;

    short tie = 0; // flag {0 = No Tie exists, 1 = Tie exists}
    
    // As long as next process has same arrival as current process keep pushing in heap
    while (pTemp->ArrivalTime == pCurrentProcess->ArrivalTime) 
    {
        tie = 1; //tie exists
        // [1] Dequeue  {Remove the next process that have the same arrival time as the current process}
        ProcDequeue(Process_Queue, &pTemp);
        // [2] Arrange  {Push the next process into the heap to be sorted according to priority}
        HeapPush(Process_Heap, pTemp->Priority, pTemp);
        // [3] Enqueue  {Get the next process in processes queue which has the hiest priority}
        ProcPeek(Process_Queue, &pTemp); 
    } // All tied next process are pushed
    
    // Now push the current process in the heap as well
    if (tie) //if a tie exists push current process
        HeapPush(Process_Heap, pCurrentProcess->Priority, pCurrentProcess);

    // Now that all processes with tie are in the heap
    return tie;
}
