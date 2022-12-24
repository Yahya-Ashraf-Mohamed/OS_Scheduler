#include "Header_File/headers.h"
#include "Header_File/Process_Struct.h"
/* Modify this file as needed*/
int remainingtime;
int main(int agrc, char * argv[])
{
    initClk();

    //=============================================================

    //arguments of this file is sent from algorithm file currently running
    //atoi converts string argument to integer 
    remainingtime = atoi(argv[1]);
    int current_time = 0;
    int previous_time = getClk();

    while (remainingtime > 0) 
    {
        current_time = getClk();

        //detecting clk increment
        if (previous_time < current_time) 
        {
            remainingtime--; 
            previous_time = current_time;
        }

        printf("(process.c) Remaining time: %d\n", remainingtime);

    }

    if(remainingtime == 0)
        exit(1); //exit with exit code 1 (shild signal is sent to parent)

    //=============================================================
    destroyClk(false);
    
    return 0;
}



