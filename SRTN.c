#include <unistd.h>
#include <string.h>
#include "Header_File/Message_Buffer.h"
#include "Header_File/headers.h"
#include "Header_File/Events_Queue.h"
#include "Header_File/Event_Struct.h"
#include "Header_File/Min_Heap.h"
#include "Header_File/Process_Queue.h"
//#include "Header_File/SRTN.h"
//================================================================================
/**/void SRTN_start(struct Processes *process_ptr);
void SRTN_stop(struct Processes *process_ptr);
void SRTN_resume(struct Processes *process_ptr);
void SRTN_finish_handler(int signum);
struct Processes * SRTN_compare(struct Processes *process_ptr , struct Processes *process_running);
unsigned int SRTN_RemaininTime(struct Processes *process_ptr );
void InitIPC();
void Process_Arrival_Handler(int signum);
int Receive_Process();
void AddEvent(struct Processes *process_ptr,enum EventType Etype);
void Log_AllEvents(unsigned int start_time, unsigned int end_time);
void SRTN_clear_resources();
void SRTN_excecute();//*/
//================================================================================
//queues                 
PriorityQueue ReadyQueue;                                       // Ready processes priority queue arranged ascendingly by remaining time
PriorityQueue *pReadyQueue = &ReadyQueue;                       // ReadyQueue pointer

queue FinishedQueue;                                            // Queue of finished processes
event_queue EventQueue;                                        //queue of events {START,STOP,RESUMED,FINISH}

// process pointers
PriorityQueue_data *Pprocess = NULL;
struct Processes *pRunning = NULL;
struct Processes *pRunningNext = NULL;

//variables
int isRunning = 0;                                             // returns 1 if there is a process currently running
int issending = 1;  //need to take number of process            // 1->all processes diddn't arrive yet  0->all processes arrived
int Received_MsgQueue_Id;
//================================================================================
void SRTN_start(struct Processes *process_ptr)
{
    pRunning = process_ptr;
    pRunning->RemainTime =  pRunning->Runtime;
    pRunning->startBefore = 1;
    pRunning->StartTime = getClk();
    pRunning->ResumeTime = getClk();
    isRunning = 1;

    //process will exit ready queue to start running
    dequeue_PriorityQueue(pReadyQueue);

    // Create new process
    int pid = fork();
    pRunning->mPid = pid;

    //fork error
    if (pid == -1) 
    {
        perror("Error in fork (SRTN_start)");
        exit(-1);
    }
 

    // Child (new process)
    if (pid == 0) 
    {

        printf("(SRTN_start) A process created with pid = %d \n", getpid());

        // remaining time must be string to be able to send it as an argument to process file
        // Convert the remaining time to string
        char RemaininTime_str[50];
        tostring(RemaininTime_str,pRunning->RemainTime);
        //sprintf(RemaininTime_str, "%d", pRunning->RemainTime);
        

        // calling execvp system call to replace the image of the child with process file
        // sending remaining time as an argument to the process file
        char* command = "./process.out";
        char* argument_list[] ={"./process.out", RemaininTime_str, NULL};
        int status_code = execvp(command,argument_list);

        //execvp error
        if (status_code == -1) 
        {
            perror("Error in execvp (SRTN_start)");
            exit(-1);
        }
    }


}
//=================================================================================
void SRTN_stop(struct Processes *process_ptr)
{
  process_ptr->LastStop = getClk();
  process_ptr->RemainTime = SRTN_RemaininTime(process_ptr);
  isRunning = 0;
  pRunning = NULL;

  int pid = process_ptr->mPid;
  kill(pid,SIGSTOP);

  // process will return back to ready queue
  enqueue_PriorityQueue(pReadyQueue,process_ptr->RemainTime, process_ptr);

}
//=================================================================================
void SRTN_resume(struct Processes *process_ptr)
{
    pRunning = process_ptr;
    pRunning->ResumeTime = getClk();
    isRunning = 1;

    //process will exit ready queue to resume running
    dequeue_PriorityQueue(pReadyQueue);

    int pid = getpid();
    kill(pid,SIGCONT);

    if(pid == pRunning->mPid)
    {
        printf("(SRTN_resume) A process with pid = %d resumed \n", getpid());

        // remaining time must be string to be able to send it as an argument to process file
        // Convert the remaining time to string
        char RemaininTime_str[50];
        //tostring(RemaininTime_str,pRunning->RemainTime);
        sprintf(RemaininTime_str, "%d", pRunning->RemainTime);
        

        // calling execvp system call to replace the image of the child with process file
        // sending remaining time as an argument to the process file
        char* command = "./process.out";
        char* argument_list[] ={"./process.out", RemaininTime_str, NULL};
        int status_code = execvp(command,argument_list);

        //execvp error
        if (status_code == -1) 
        {
            perror("Error in execvp (SRTN_resume)");
            exit(-1);
        }
    
    }
}
//==================================================================================
//done still finish event
void SRTN_finish_handler(int signum) //move to scheduler
{
    //setting values
    pRunning->FinishTime = getClk();
    pRunning->LastStop = getClk();
    pRunning->RemainTime = 0;
    isRunning = 0;
    pRunning = NULL;

    // other calculations here

    // move process to finished processes queue
    ProcEnqueue(FinishedQueue,pRunning);

    //Event finished
    AddEvent(pRunning,FINISH);

    int status;                     // exit code of child
    int wpid = wait(&status);       // pid of waited child who sent the signal

    // sending sigkill signal to the process 
    kill(wpid, SIGKILL);

  signal(SIGCHLD, SRTN_finish_handler);  
}
//===================================================================================
struct Processes * SRTN_compare(struct Processes *process_ptr , struct Processes *process_running)
{
    unsigned int process_remaintime = process_ptr->RemainTime;
    unsigned int running_process_remaintime = SRTN_RemaininTime(process_running);

    //new process has smaller remaintime
    if(process_remaintime < running_process_remaintime)
        return process_ptr;
    //running process has smaller remaintime
    else if (process_remaintime > running_process_remaintime)
        return process_running;
    // if both have same remaining time running process will continue
    else if (process_remaintime == running_process_remaintime) 
        return process_running;
}
//===================================================================================
// remaining time of current running process = Remaining time in process struct - (current time - resumed time)
unsigned int SRTN_RemaininTime(struct Processes *process_ptr )
{
    
    unsigned int remaining_t;
    remaining_t = process_ptr->RemainTime - (getClk() - process_ptr->ResumeTime);
    //notes:
    //when assigning start time assign it to resumed time also
    //when assigning run time assigne it to remaining time also
    return remaining_t;

}
//============================================================= Dealing with message queue ======================================
// Initialize the IPC and Connect it the process_generator
void InitIPC()
{
    key_t key = ftok(Ftok_File, Ftok_Key); //same parameters used in process_generator
    Received_MsgQueue_Id = msgget(key, 0);
    if (Received_MsgQueue_Id == -1) {
        perror("SRTN: SRTN IPC init failed!");
        raise(SIGINT);
    }
    printf("SRTN: SRTN IPC ready...\n");
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
        perror("SRTN: Error while receiving!");
        return -1;
    }

    // if message was retrieved from the message queue
    printf("SRTN: Received by SRTN scheduler\n");
    struct Processes *pProcess = malloc(sizeof(Process));    // allocate memory for the received process
    while (!pProcess) {
        perror("SRTN: Malloc Failed!");
        printf("SRTN: Trying Again...");
        pProcess = malloc(sizeof(Process));
    }
    *pProcess = msg.mProcess; //store the process received in the allocated space
    enqueue_PriorityQueue(pReadyQueue,pProcess->Runtime,pProcess);//push the process pointer into the ready process priority queue queue
    return 0;
}
//============================================================= Dealing with Events ======================================
// in each process AddEvent(#state of the event {START,STOP,RESUMED,FINISH});
void AddEvent(struct Processes *process_ptr,enum Event_Type Etype)
{
    Event *pCurrentEvent = malloc(sizeof(Event));
    while (!pCurrentEvent) {
        perror("SRTN: Malloc failed");
        printf("SRTN: Trying again");
        pCurrentEvent = malloc(sizeof(Event));
    }

    pCurrentEvent->Time_Step = getClk();
    if (Etype == FINISH) {
        pCurrentEvent->Turn_Around_Time = getClk() - process_ptr->ArrivalTime; 
        pCurrentEvent->Weight_Turn_Around_Time = (double) pCurrentEvent->Turn_Around_Time / process_ptr->Runtime;
    }
    pCurrentEvent->pProcess = process_ptr;
    pCurrentEvent->Current_Wait_Time = process_ptr->WaitTime;  //Need to be discussed(wait time:turnaround -start)(wastedtime=turnaround-run)
    pCurrentEvent->Type = Etype;
    pCurrentEvent->Current_Remaining_Time = process_ptr->RemainTime;
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
    while (EventQueueDequeue(EventQueue, &pNextEvent))
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
//===================================================================================
void SRTN_clear_resources()
{
    struct Processes *Pprocess = NULL;   // deallocate before the pointer holds any process
    while (ProcDequeue(FinishedQueue, Pprocess)) //while processes queue is not empty
        free(Pprocess); //free memory allocated for this process

    Event *pEvent = NULL;  // deallocate before the pointer holds any event
    while (EventQueueDequeue(EventQueue, &pEvent)) //while event queue is not empty
        free(pEvent); //free memory allocated by the event
    
    destroy_PriorityQueue(pReadyQueue);
    if(pRunning)
        free(pRunning);
    if(pRunningNext)
        free(pRunningNext);
}
//===================================================================================
void SRTN_excecute() //will be moved to scheduler
{
    //initializing queues
    PriorityQueue ReadyQueue = NewPriorityQueue(pReadyQueue, 0);    
    queue FinishedQueue = NewProcQueue();  
    EventQueue = NewEventQueue();


    initClk(); //initialize clock
    InitIPC(); // Creat and Connect message queue with the Message queue of the process_generator

    //signal sent from child will be handled by SRTN_finish_handler 
    signal(SIGCHLD, SRTN_finish_handler);
    //signal sent from process generator (when new process arrive) will be handled by Process_Arrival_Handler 
    signal(SIGUSR1, Process_Arrival_Handler); 
    //handling interrupt signal
    signal(SIGINT, SRTN_clear_resources()); 
    
    pause(); // Wait for the first process to arrive
    unsigned int start_time = getClk();     // Get the start time

    //this while will run until all processes arrive and finish excecuting
    while ( issending || (Pprocess = peek_PriorityQueue(pReadyQueue)) !=  NULL )
    //while ((Pprocess = peek_PriorityQueue(pReadyQueue)) !=  NULL || finished < no_process)
    {
        if(Pprocess !=  NULL)
        {
            
            if(!isRunning) //if there is no running process currently
            {
                if(!(Pprocess-> startBefore)) //check if the process didn't run before (first time for process to run)
                {
                    SRTN_start(Pprocess);
                    AddEvent(Pprocess,START);
                }
                else
                {
                    SRTN_resume(Pprocess);
                    AddEvent(Pprocess,RESUMED);
                }

                }
            else
            {

                pRunningNext = SRTN_compare(Pprocess,pRunning);
                if (pRunning != pRunningNext)
                {
                    //stop running process  
                    if(SRTN_current_RemaininTime(pRunning))
                    {
                      SRTN_stop(pRunning);
                      AddEvent(pRunning,STOP);
                    }
                    //if process is finished process exit on her own
                        
                        

                    //running next process can start or resume
                    if(!(pRunningNext-> startBefore)) //check if the process didn't run before (first time for process to run)
                    {
                        SRTN_start(pRunningNext);
                        AddEvent(pRunningNext,START);
                    }

                    else
                    {
                        SRTN_resume(pRunningNext);
                        AddEvent(pRunningNext,RESUMED);
                    }

                
                }
            }
        } 
    }  //while loop

    SRTN_clear_resources();

    unsigned int finish_time = getClk();    // Get the finish time
    Log_AllEvents(start_time, finish_time);
    raise(SIGINT);  // Clean & Exit

}  //SRTN_excecute()

//===================================================================================


