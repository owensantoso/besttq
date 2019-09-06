#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

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
//int ionumbers[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS][2]; // start time (microsec), bytes to transfer
int iostart[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};    // start time (microsec)
int iobytes[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};     // bytes to transfer
int ioruntimes[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};  // time for each io to run
char iodevice[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS][MAX_DEVICE_NAME];   // device names of each io request
int  devicecount = 0;
int  processcount = 0;
int  iocount = 0;


//  ----------------------------------------------------------------------

#define CHAR_COMMENT            '#'
#define MAXWORD                 20

void parse_tracefile(char program[], char tracefile[])
{
    // Initialise iostart to -1 (done)
    for(int i = 0; i < MAX_PROCESSES; i++){
        for(int j = 0; j < MAX_EVENTS_PER_PROCESS; j++){
            iostart[i][j] = -1;
        }
    }

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
            iostart[processcount][iocount] = atoi(word1); //Store execution time
            iobytes[processcount][iocount] = atoi(word3); //Store amount of data transferred 
            strcpy(iodevice[processcount][iocount], word2);
            
            // Calculate how long each io runs for
            for(int i = 0; i < devicecount; i++){
                if(strcmp(iodevice[processcount][iocount],devices[i])==0){
                    ioruntimes[processcount][iocount] = (int) 1000000*((long) atoi(word3))/devicespeeds[i]          // convert to long since it goes over int limit
                                                          + ((1000000*(long) atoi(word3) % devicespeeds[i]) != 0);  // rounds up
                    break;
                }
            }


            iocount++;
        }

        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
            ;   //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
            processtimes[processcount][2] = atoi(word1);  //Store execution time of process 
            processcount++;
            iocount = 0;
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

/*
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
*/


#define SPACE 5

int readyqueue[MAX_PROCESSES]; // Queue for processes that are ready to be run
int rqsize = 0; // Size of ready queue, used for indexing 
int blqueue[MAX_PROCESSES];   // Blocked queue, waiting for slow i/o
int blqueuesize = 0;    //Size of blocked queue, used for indexing 
int runningprocessindex = -1;    // when running process is -1, it means no processes are running
int time = 0;
int timeleft[MAX_PROCESSES] = {0};
int iotimelefttostart[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};
int currenttq = 0;  // Tracks the current time quantum 
bool databusfree = true;
int runningioprocess = -1;
int runningionumber = -1;
int nexit = 0;


// prints readyqueue, running and nexit
void printrq(int numtabs){
    int i=0;
    for(int j = 0; j < numtabs; j++){
        printf("\t"); 
    }
    printf("RQ=["); 
    do{
        printf("%i", processtimes[readyqueue[i]][0]);
        i++;
    }while(readyqueue[i] != -1);
    //for(int i = 0; i<MAX_PROCESSES;i++){
    //    printf("%i", readyqueue[i]);
    //}
    printf("]\t");
    printf("running=p%i   ", processtimes[runningprocessindex][0]);
    printf("nexit=%i\n", nexit);
}


// Move queue forward
void qforward(void){
    for (int i = 0; i < MAX_PROCESSES-1; i++)
    {
        readyqueue[i] = readyqueue[i+1];        // move queue forward
    }
    readyqueue[MAX_PROCESSES-1] = -1;            // sets end of queue to 0
    rqsize--;
    return;
}

void blqforward(void){
    for (int i = 0; i < MAX_PROCESSES-1; i++)
    {
        blqueue[i] = blqueue[i+1];              // move blqueue forward
    }
    blqueue[MAX_PROCESSES-1] = 0;               // sets end of queue to 0
    blqueuesize--;
    return;
}

// Checks if any new or previously blocked processes are ready
void checkready(void){
    for(int i = 0;i < processcount; i++){
        if(time == processtimes[i][1]){
            readyqueue[rqsize] = i;     // add next process to readyqueue
            printf("time: %i\t p%i.NEW->READY", time, processtimes[i][0]); 
            printrq(SPACE+1);
            rqsize++;
        }
    }

}


// Requests use of databus if there are any io queued
void checkblqueue(void){
    if(databusfree && blqueue[0] != -1){
        databusfree = false;            // occupy databus
        runningioprocess = blqueue[0];  // set running io process number to start of blqueue
        printf("time: %i\t p%i.request_databus", time, processtimes[runningioprocess][0]); 
        printrq(SPACE);
        for(int i = 0; i < MAX_EVENTS_PER_PROCESS; i++){
            if(iotimelefttostart[runningioprocess][i] == 0){
                runningionumber = i;    // set running io number
                break;
            }
        }
        for (int i = 0; i < 5; i++){
            time++;                  // 5 usecs to acquire databus
            checkready();                               
        }
        blqforward();
    }
}

void releasedatabus(void){
    printf("time: %i\t p%i.release_databus", time, processtimes[runningioprocess][0]); 
    printrq(SPACE);
    iotimelefttostart[runningioprocess][runningionumber] = -1;
    databusfree = true;
    runningioprocess = -1;
    runningionumber = -1;
}


// Add a specific process to the ready queue
void addtorq(int process){
    readyqueue[rqsize] = process;
    rqsize++;
}

void addtoblq(int process){
    blqueue[blqueuesize] = process;
    blqueuesize++;
}

// Updates times and io device times
void updatetime(void){
    time++; 
    if (runningprocessindex != -1) { // if there is a running process, tick relevant times down
        currenttq--;  
        timeleft[runningprocessindex]--;
        for(int i = 0; i < MAX_EVENTS_PER_PROCESS; i++){
            if(iotimelefttostart[runningprocessindex][i] <= 0){    // if time is zero, don't tick down
                continue;
            }
            iotimelefttostart[runningprocessindex][i]--; // tick io times down for current process
        }
    }
    // io checks
    checkblqueue();
    if(databusfree == false){   // if databus is being used
        if(ioruntimes[runningioprocess][runningionumber] == 1){ // if io will complete in the next usec
            addtorq(runningioprocess);
            releasedatabus();
        }
        else{
            ioruntimes[runningioprocess][runningionumber]--;
        }
    }      
    //printf("%i\n", databusfree);    
}

// Move from ready to running if there is no currently running process
void checkrunning(void){
    if(runningprocessindex == -1 && readyqueue[0] != -1){
        for (int i = 0; i < 5; i++){
            updatetime();                                      // 5 usecs to change from READY->RUNNING
            checkready();                               
        }
        runningprocessindex = readyqueue[0];                 // set runningprocessindex to start of readyqueue
        qforward();
        printf("time: %i\t p%i.READY->RUNNING", time, processtimes[runningprocessindex][0]);
        printrq(SPACE);
    }
}

void exitprocess(void){
    nexit++;
    printf("time: %i\t p%i.RUNNING->EXIT", time, processtimes[runningprocessindex][0]);  
    runningprocessindex = -1;
    printrq(SPACE);  
}

// Checks if the current running process has remaining execution time
bool isfinished(void){ 
    if(timeleft[runningprocessindex] <=0){
        return true;
    }
    return false;
}

// Blocks running process
void blockprocess(int ionum){
    addtoblq(runningprocessindex);
    printf("time: %i\t p%i.RUNNING->BLOCKED(%s)", time, processtimes[runningprocessindex][0],iodevice[runningprocessindex][ionum]);  
    runningprocessindex = -1;                     // stops running process
    printrq(SPACE-1);
}

// Checks if running process needs any io
int checkio(void){
    for(int i = 0; i < MAX_EVENTS_PER_PROCESS; i++){
        if(iotimelefttostart[runningprocessindex][i] == 0){
            return i;
        }
    }
    return -1;      // return -1 if no io needed
}



//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
    for (int i =0; i<MAX_PROCESSES; i++) { // initialise ready queue to empty (-1)
        readyqueue[i] = -1; 
        blqueue[i] = -1;
    }
    for(int i = 0; i < processcount; i++){
        timeleft[i] = processtimes[i][2]; //should it be start time
    }
    memcpy(iotimelefttostart, iostart, sizeof (int) * MAX_PROCESSES * MAX_EVENTS_PER_PROCESS); // copy iostart times into iotimeleft array
    /*
    printf("iostart: \n");
    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10; j++){
            printf("%i ",iostart[i][j]);
        }
        printf("\n");
    }
    printf("iotimeleft: \n");
    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10; j++){
            printf("%i ",iotimelefttostart[i][j]);
        }
        printf("\n");
    }
    
    printf("iobytes: \n");
    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10; j++){
            printf("%i ",iobytes[i][j]);
        }
        printf("\n");
    }
    printf("ioruntimes: \n");
    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10; j++){
            printf("%i ",ioruntimes[i][j]);
        }
        printf("\n");
    }
    */
    currenttq = time_quantum;
    //printf("processcount: %i\n", processcount);
    printf("time: %i  \t reboot with TQ = %i\n", time, time_quantum);

    while(nexit < processcount){                        // while there are still processes to run
        //printf("BQ %i\n", blqueue[0]);
        while(time < processtimes[nexit][1] || databusfree == false){          // increment time until process start time
            //printf("time: %i\n",time);
            updatetime();
            //printf("time: %i\t ioruntimes: %i\n", time,ioruntimes[0][0]);
        }
        // printf("rqsize: %i\n",rqsize);
        //printf("currenttq: %i\n", currenttq);
        //printf("runningprocessindex: %i\n", runningprocessindex);
        checkready();
        checkrunning();
        //printf("runningprocessindex: %i\n", runningprocessindex);
        //printf("p1: %i, p2: %i, p3: %i, p4: %i, p5: %i, p6: %i, p7: %i, p8: %i\n", 
        //                timeleft[0],timeleft[1],timeleft[2],timeleft[3],timeleft[4],timeleft[5],timeleft[6],timeleft[7]);
        while (timeleft[runningprocessindex] > 0)                     // loop until process has no time remaining
        {
            //printf("currenttq: %i\n", currenttq);
            currenttq = time_quantum;                   // the current TQ must be reset
            while(currenttq > 0)                        // loop until end of current TQ
            {
                //printf("p1: %i, p2: %i, p3: %i, p4: %i, p5: %i, p6: %i, p7: %i, p8: %i\n", 
                //        timeleft[0],timeleft[1],timeleft[2],timeleft[3],timeleft[4],timeleft[5],timeleft[6],timeleft[7]);
                //printf("iotimeleft: %i\n",iotimelefttostart[0][0]);
                updatetime();       
                checkready();
                int ionum;
                if((ionum = checkio()) != -1){    // if the process needs io
                    currenttq = time_quantum;   // reset time quantum
                    blockprocess(ionum);        // block it
                    checkblqueue();
                    checkrunning();             // run next process
                    goto endfunc;
                }
                if(isfinished()){               // if process has terminated
                    currenttq = time_quantum;   // reset time quantum
                    exitprocess();
                    goto endfunc;
                }
            }                                           // once this TQ is over,
            

            if(readyqueue[0] != -1){             // if there is a process waiting
                addtorq(runningprocessindex);
                printf("time: %i\t p%i.expire,   p%i.RUNNING->READY", time, processtimes[runningprocessindex][0],processtimes[runningprocessindex][0]);
                runningprocessindex = -1;             // stop current process 
                printrq(SPACE-1);
                checkrunning();
                printf("p1: %i, p2: %i, p3: %i, p4: %i, p5: %i, p6: %i, p7: %i, p8: %i\n", 
                        timeleft[0],timeleft[1],timeleft[2],timeleft[3],timeleft[4],timeleft[5],timeleft[6],timeleft[7]);
            }
            else{
                printf("time: %i\t p%i.freshTQ", time, processtimes[runningprocessindex][0]); 
            printrq(SPACE+1);
            } 
        }
        exitprocess();
        endfunc:; //For goto function
    }

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
