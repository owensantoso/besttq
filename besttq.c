#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CITS2002 Project 1 2019
   Name(s):             Owen Santoso, Victor Jongue
   Student number(s):   22466085, 22493718
 */

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c


//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES             4
#define MAX_DEVICE_NAME         20  
#define MAX_PROCESSES           50
#define MAX_EVENTS_PER_PROCESS	100

#define TIME_CONTEXT_SWITCH     5
#define TIME_ACQUIRE_BUS        5


//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum                = 0;
int total_process_completion_time       = 0;

// parsed information will go into these variables
char devices[MAX_DEVICES][MAX_DEVICE_NAME];          // device name, transfer speed (bytes/sec)
int devicespeeds[MAX_DEVICES];
int processtimes[MAX_PROCESSES][3];    // process number, start time (microsec), end time (microsec)
int ionumbers[MAX_EVENTS_PER_PROCESS*MAX_PROCESSES][3]; // process number, start time (microsec), bytes to transfer
char iodevice[MAX_EVENTS_PER_PROCESS*MAX_PROCESSES][MAX_DEVICE_NAME];   // device names
int  devicecount = 0;
int  processcount = 0;
int  iocount = 0;


//  ----------------------------------------------------------------------

#define CHAR_COMMENT            '#'
#define MAXWORD                 20

void parse_tracefile(char program[], char tracefile[])
{
//  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp    = fopen(tracefile, "r");

    if(fp == NULL) {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int  lc     = 0;

//  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while(fgets(line, sizeof line, fp) != NULL) {
        ++lc;

//  COMMENT LINES ARE SIMPLY SKIPPED
        if(line[0] == CHAR_COMMENT) {
            continue;
        }

//  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char    word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

//  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if(nwords <= 0) {
            continue;
        }
//  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENTS
        if(nwords == 4 && strcmp(word0, "device") == 0) {
            strcpy(devices[devicecount], word1);    //Stores the name of the device
            devicespeeds[devicecount] = atoi(word2);        //Stores the speed of device (byte/s)
            devicecount++;
        }

        else if(nwords == 1 && strcmp(word0, "reboot") == 0) {
            ;   // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }
// FOUND THE START OF A PROCESS'S EVENTS
        else if(nwords == 4 && strcmp(word0, "process") == 0) {
            processtimes[processcount][0] = atoi(word1);  //Store process number   
            processtimes[processcount][1] = atoi(word2);  //Store process start time
        }
 //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
        else if(nwords == 4 && strcmp(word0, "i/o") == 0) {
            ionumbers[iocount][0] = processtimes[processcount][0]; //Process number
            ionumbers[iocount][1] = atoi(word1); //Store execution time
            ionumbers[iocount][2] = atoi(word3); //Store amount of data transferred 
            strcpy(iodevice[iocount], word2);
            iocount++;
        }

        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
            ;   //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
            processtimes[processcount][2] = atoi(word1);  //Store execution time of process 
            processcount++;
        }

        else if(nwords == 1 && strcmp(word0, "}") == 0) {
            ;   //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
        }
        else {
            printf("%s: line %i of '%s' is unrecognized",
                        program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
}

#undef  MAXWORD
#undef  CHAR_COMMENT


//Test function to make sure arrays are storing correct information 
void print_tracefile(void) 
{
    for (int i = 0; i<devicecount; i++)
    {
        printf("Device\t %s\t %i\n", devices[i], devicespeeds[i]);
    }
    for (int i = 0; i<processcount; i++)
    {
        printf("Process\t %i\t %i\t %i\n", processtimes[i][0], processtimes[i][1], processtimes[i][2]);
    }
    for (int i = 0; i<iocount; i++)
    {
        printf("i/o\t %i\t %s\t %i\t\t (Process: %i)\n", ionumbers[i][1], iodevice[i], ionumbers[i][2], ionumbers[i][0]);
    }

}

// optimal_time_quantum
// total_process_completion_time
// char *devices[MAX_DEVICES][2];          // device name, transfer speed (bytes/sec)
// int  processtimes[MAX_PROCESSES][3];    // process number, start time (microsec), end time (microsec)
// int ionumbers[MAX_EVENTS_PER_PROCESS*MAX_PROCESSES][3]; // process number, start time (microsec), bytes to transfer
// char *iodevice[MAX_EVENTS_PER_PROCESS*MAX_PROCESSES];   // device names
int readyqueue[MAX_PROCESSES] = {0};
int rqsize = 0;
// int emptyqueue[MAX_PROCESSES] = {0};
// int multiblqueue[MAX_DEVICE_NAME][MAX_PROCESSES] = {0};
int runningprocess = 0;
int time = 0;
int timeleft[MAX_PROCESSES] = {0};
int currenttq = 0;
//int timeuntilnextio = 0;
//int lastiotime = 0;
int nexit = 0;


// Move queue forward
void qforward(void){
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
            readyqueue[i] = readyqueue[i+1];            // move queue forward
    }
    rqsize--;
    return;
}

// Updates time
void updatetime(void){
    currenttq--;                            // decrease current TQ by 1
    timeleft[nexit]--;                      // decrease time left by 1
    time++;                                 // increment time by 1
}


// Checks if any new processes are ready
void checkready(void){
    if(runningprocess == 0){
        if(time == processtimes[nexit][1]){
            readyqueue[rqsize] = processtimes[nexit][0];         // add process to readyqueue
            printf("time: %i\t p%i.NEW->READY\n", time, processtimes[nexit][0]); 
            rqsize++;
        }
    }
    else if(time == processtimes[nexit+1][1]){
        readyqueue[rqsize] = processtimes[nexit][0];         // add process to readyqueue
        printf("time: %i\t p%i.NEW->READY\n", time, processtimes[nexit+1][0]); 
        rqsize++;
    }
}

// ready to running if there is no running process
void checkrunning(void){
    if(runningprocess == 0){
        time += 5;                                      // 5 usecs to change from READY->RUNNING
        runningprocess = readyqueue[0];                 // set runningprocess to start of readyqueue
        printf("time: %i\t p%i.READY->RUNNING\n", time, processtimes[nexit][0]);
        qforward();
    }
}


//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
    for(int i = 0; i < processcount; i++){
        timeleft[i] = processtimes[i][processcount];
    }
    currenttq = time_quantum;
    printf("processcount: %i\n", processcount);
    printf("time: %i  \t reboot with TQ = %i\n", time, time_quantum);

    while(nexit < processcount){                        // while there are still processes to run
        while(time < processtimes[nexit][1]){          // increment time until process start time
            time++;
            //printf("time: %i\n",time);
        }
        // printf("rqsize: %i\n",rqsize);

        checkready();
        checkrunning();

        while (timeleft[nexit] > 0)                     // loop until process has no time remaining
        {
            while(currenttq > 0)                        // loop until end of current TQ
            {
                updatetime();       
                checkready();
            }                                           // once this TQ is over,
            currenttq = time_quantum;                   // the current TQ must be reset
            printf("runningprocess: %i\n", runningprocess);
            /*
            if (timeleft[nexit] != 0 && readyqueue[0] == 0)    // if there is time left, start new TQ (print)
            {
                printf("time: %i\t p%i.freshTQ\n", time, processtimes[nexit][0]); 
            }
            */

            if(readyqueue[0] != 0){             // if there is a process waiting
                runningprocess = 0;             // stop current process
                checkrunning();
                checkready();

                readyqueue[rqsize] = processtimes[nexit][0];         // add process to readyqueue
                printf("time: %i\t p%i.expire,\t p%i.RUNNING->READY\n", time, processtimes[nexit+1][0],runningprocess); 
                rqsize++;
            }
            else{
                printf("time: %i\t p%i.freshTQ\n", time, processtimes[nexit][0]); 
            }          
            if(runningprocess == 0){}
            
        }
        printf("time: %i\t p%i.RUNNING->EXIT\n", time, processtimes[nexit][0]); 
        runningprocess = 0;
        nexit++;
    }

/*
    time = processtimes[1][1];
    printf("time: %i\t p%i.new->ready\n", time, processtimes[1][0]); 
    readyqueue[0] = processtimes[1][0];
    runningprocess = readyqueue[0];
    readyqueue[0] = 0;
    time += 5;
    printf("time: %i\t p%i.ready->running\n", time, processtimes[1][0]); 
    while (timeleft[1] > 0){
        while(currenttq > 0){
            currenttq--;
            timeleft[1]--;
            time++;
        }
        currenttq = time_quantum;
        printf("time: %i\t p%i.freshTQ\n", time, processtimes[1][0]); 
    }
    printf("time: %i\t p%i.running->exit\n", time, processtimes[1][0]); 
*/

/*
    while(runningprocess != 0 || memcmp(readyqueue, emptyqueue, MAX_PROCESSES) != 0){
        if(runningprocess == 0 && readyqueue[0] == 0){
            time = processtimes[0][1];
            printf("Ready time: %i", time);
            time += 5;
            continue;
        }
        
        time++;
    }
    */


    printf("running simulate_job_mix( time_quantum = %i usecs )\n",
                time_quantum);
}



void usage(char program[])
{
    printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    int TQ0 = 0, TQfinal = 0, TQinc = 0;

//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND THREE TIME VALUES
    if(argcount == 5) {
        TQ0     = atoi(argvalue[2]);
        TQfinal = atoi(argvalue[3]);
        TQinc   = atoi(argvalue[4]);

        if(TQ0 < 1 || TQfinal < TQ0 || TQinc < 1) {
            usage(argvalue[0]);
        }
    }
//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND ONE TIME VALUE
    else if(argcount == 3) {
        TQ0     = atoi(argvalue[2]);
        if(TQ0 < 1) {
            usage(argvalue[0]);
        }
        TQfinal = TQ0;
        TQinc   = 1;
    }
//  CALLED INCORRECTLY, REPORT THE ERROR AND TERMINATE
    else {
        usage(argvalue[0]);
    }

//  READ THE JOB-MIX FROM THE TRACEFILE, STORING INFORMATION IN DATA-STRUCTURES
    parse_tracefile(argvalue[0], argvalue[1]);

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
//  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
//  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED

    for(int time_quantum=TQ0 ; time_quantum<=TQfinal ; time_quantum += TQinc) {
        simulate_job_mix(time_quantum);
    }


    //print_tracefile();

//  PRINT THE PROGRAM'S RESULT
    printf("best %i %i\n", optimal_time_quantum, total_process_completion_time);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
