#include "Header_File/headers.h"
#include "Header_File/Process_Struct.h"
#include "Header_File/Process_Heap.h"
#include "Header_File/Message_Buffer.h"
#include "Header_File/Events_Queue.h"
#include "Header_File/Process_Queue.h"

//Functions headers
void initIPC();
void ExcuteProcess();
int Receive_Process();
void Clean_Resources(int signum);
void AddEvent(enum Event_Type Etype);
void Process_Termination(int signum);
void Log_AllEvents(unsigned int start_time, unsigned int end_time);
void Process_Arrival_Handler(int signum);


//Global Parameters
event_queue EventQueue; //Event Queue
int recievedMsgQueueId =0;  //ID of MsgQueue to recieve process from process generator
unsigned short switchContextFlag =0; //switch context flag {1 = UNLOCKED or switch, 0 = LOCKED or no switch}
unsigned int allProcessCount = 0; //#processes, initially
unsigned int recievedProcessCount=0;
bool isAllRecieved = 0;
Process *pCurrentProcess =NULL;   //Pointer to the current running process
heap_t *ProcessPQueue;

int main(int argc, char *argv[])
{
   printf("SJF: SJF scheduling Algorithm started\n");
   
   //Initializing CLK,  IPCs & dat
   initClk();      //Inisialize clock
   initIPC();      //Inicialze mesaageQueue IPC to recieve processes from process generator

   allProcessCount = atoi(argv[1]); 
   EventQueue = NewEventQueue();   //Initialize Event Queue
   ProcessPQueue = (heap_t *) calloc(1, sizeof(heap_t));

   //link Signal handlers to signals
   signal(SIGUSR1, Process_Arrival_Handler);
   signal(SIGINT, Clean_Resources); //handle SIGINT to Clean resources
   signal(SIGCHLD, Process_Termination); //handle when a process finishes execution using ChildHandler

   //SJF Excution//
   pause();    //suspend until the next signal arrives
   unsigned int start_time = getClk();
   while (1)
   {
       //Recieving the process from the process generator
       pause();
       //Check if the critical section is unlocked && there are ready processes in the system
       if (switchContextFlag && !HeapEmpty(ProcessPQueue))
       {
           switchContextFlag = 0; //Critical Section is now Locked
           pCurrentProcess = HeapPop(ProcessPQueue);
           ExcuteProcess();
       }
       // Check if all processes recieved or not
       //Check if all processes are finished
       if (isAllRecieved && HeapEmpty(ProcessPQueue) && switchContextFlag)
           break;
   }
   unsigned int end_time = getClk();
   Log_AllEvents(start_time, end_time); 
   raise(SIGINT);  // Clean & Exit

   return 0;
}

//Initialize IPC and connect to Process Genrator
void initIPC()
{
   key_t key = ftok(Ftok_File, Ftok_Key); //Same parameters as used in Process Generator
   recievedMsgQueueId = msgget(key, 0);
   if (recievedMsgQueueId == -1) {
       perror("SJF: IPC initialization failed!");
       raise(SIGINT);  //Clean & Exit
   }
   printf("SJF: MessageQueue IPC ready!\n");
}

int Receive_Process()
{
   Message msg;
   // Error in recieving the MsgQueue
   if (msgrcv(recievedMsgQueueId, (void *) &msg, sizeof(msg.mProcess), 0, IPC_NOWAIT) == -1)
   {
       perror("SJF: Error while receiving!");
       return -1;
   }

   // if message was retrieved from the message queue


   printf("SJF: Received by SJF scheduler\n");


   Process *pProcess = malloc(sizeof(Process));    // allocate memory for the received process


   while (!pProcess) {


       perror("SJF: Malloc Failed!");


       printf("SJF: Trying Again...");


       pProcess = malloc(sizeof(Process));


   }


   *pProcess = msg.mProcess; //store the process received in the allocated space


   HeapPush(ProcessPQueue, pProcess->Runtime, pProcess);//push the process pointer into the main processes queue







   //update recievedProcessCount


   recievedProcessCount +=1;


   isAllRecieved = (allProcessCount==recievedProcessCount);







   return 0;


}







void Process_Arrival_Handler(int signum)


{


   Receive_Process();


}







void Clean_Resources(int signum)


{


   printf("SJF: Cleaning SJF scheduler resources...\n");


 


   Process *pProcess = NULL;   // deallocate before the pointer holds any process


   Event *pEvent = NULL;  // deallocate before the pointer holds any event


   while (EventQueueDequeue(EventQueue, &pEvent)) //while event queue is not empty


   {


       pProcess = pEvent->pProcess;


       free(pProcess);


       free(pEvent); //free memory allocated by the event


   }


   printf("SJF: Done! SJF Scheduler is now clean.\n");


   exit(EXIT_SUCCESS);


}












//Each process runs to completion


void ExcuteProcess()


{


   pCurrentProcess->Pid = fork();       //forking a new process


   while (pCurrentProcess->Pid ==-1)    //Error in forking


   {


       perror("SJF: Error in forking process!");


       printf("SJF: Trying to fork again...\n");


       pCurrentProcess->Pid = fork();


   }


 


   //to be excuted in the forked process itself


   if (pCurrentProcess->Pid == 0)


   {


       char buffer[10]; //buffer to convert runtime from int to string


       sprintf(buffer, "%d", pCurrentProcess->Runtime);


       char *argv[] = {"process.out", buffer, NULL};


       execv("process.out", argv);


     


       //the following will be excuted only if execv() fails.


       perror("SJF: ERROR! Process execution failed");


       exit(EXIT_FAILURE);


   }


   // to be excuted in this parent process


   switchContextFlag =0;


   //Calculate the waiting time for the current process


   pCurrentProcess->WaitTime = getClk() - pCurrentProcess->ArrivalTime;


 


   //Update the event Queue with the start of the current process


   AddEvent(START);


}












void AddEvent(enum Event_Type Etype)


{


   Event *pCurrentEvent = malloc(sizeof(Event));


   while (!pCurrentEvent) {


       perror("SJF: Malloc failed");


       printf("SJF: Trying again");


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


   EventQueueEnqueue(EventQueue, pCurrentEvent);


}












void Process_Termination(int signum)


{


   if (!waitpid(pCurrentProcess->Pid, NULL, WNOHANG)) // if current process did not terminate. return back


       return;







   pCurrentProcess->RemainTime = 0; //process finished


   AddEvent(FINISH);







   switchContextFlag = 1; //Process is terminated, and then, it's time for context switch


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


