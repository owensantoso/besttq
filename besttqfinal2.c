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


#define MAX_DEVICES             4
#define MAX_DEVICE_NAME         20  
#define MAX_PROCESSES           50
#define MAX_EVENTS_PER_PROCESS	100

#define TIME_CONTEXT_SWITCH     5
#define TIME_ACQUIRE_BUS        5


int optimal_time_quantum                = 0;
int total_process_completion_time       = 0;


// Parsed information will go into these variables
char devicenames[MAX_DEVICES][MAX_DEVICE_NAME];                         // Device name
int  devicespeeds[MAX_DEVICES];                                         // Transfer speed (bytes/sec)
char devicenames_sorted[MAX_DEVICES][MAX_DEVICE_NAME];                  // Device names sorted by decreasing speed
int  devicespeeds_sorted[MAX_DEVICES];                                  // Transfer speeds sorted by decreasing speed
int  processtimes[MAX_PROCESSES][3];                                    // Process number, Start time (microsec), Total run time (microsec)
int  iostart[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};            // Start time (microsec)
int  iobytes[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};            // Bytes to transfer
int  ioruntimes[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};         // Time for each io to run
char iodevice[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS][MAX_DEVICE_NAME];  // Device names of each io request
int  devicecount = 0;
int  processcount = 0;
int  iocount = 0;


#define CHAR_COMMENT            '#'
#define MAXWORD                 20

void parse_tracefile(char program[], char tracefile[]){
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
            strcpy(devicenames[devicecount], word1);            // Stores the name of the device
            devicespeeds[devicecount] = atoi(word2);        // Stores the speed of device (byte/s)
            devicecount++;
        }

        else if(nwords == 1 && strcmp(word0, "reboot") == 0) {
            ;   // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }
// FOUND THE START OF A PROCESS'S EVENTS
        else if(nwords == 4 && strcmp(word0, "process") == 0) {
            processtimes[processcount][0] = atoi(word1);    // Store process number   
            processtimes[processcount][1] = atoi(word2);    // Store process start time
        }
 //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
        else if(nwords == 4 && strcmp(word0, "i/o") == 0) {
            iostart[processcount][iocount] = atoi(word1);   // Store execution time
            iobytes[processcount][iocount] = atoi(word3);   // Store amount of data transferred 
            strcpy(iodevice[processcount][iocount], word2); // Store i/o device name
            
            // Calculate how long each io runs for
            for(int i = 0; i < devicecount; i++){
                if(strcmp(iodevice[processcount][iocount],devicenames[i])==0){
                    ioruntimes[processcount][iocount] = (int) 1000000*((long) atoi(word3))/devicespeeds[i]          // Convert to long since it goes over int limit
                                                          + ((1000000*(long) atoi(word3) % devicespeeds[i]) != 0);  // Rounds up
                    break;
                }
            }


            iocount++;
        }

        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
            ;   //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
            processtimes[processcount][2] = atoi(word1);    // Store execution time of process 
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

// Bubble sort algorithm for device speeds and device names
void sortdevices(void){
    // Make copy of devicespeeds array
    memcpy(devicespeeds_sorted, devicespeeds, sizeof(int)*MAX_DEVICES);

    for (int i = 0; i<MAX_DEVICES-1; i++) {
        for (int j = 0; j<MAX_DEVICES-i-1; j++){
            int temp = devicespeeds_sorted[j];                          // Variable to hold array current value
            if (devicespeeds_sorted[j] < devicespeeds_sorted[j+1]){     // Checks if the next array value less than the current
                devicespeeds_sorted[j] = devicespeeds_sorted[j+1];      // Swap the two values if so
                devicespeeds_sorted[j+1] = temp;                        
            }
        }
    }
    // Initialise the sorted device names
    for(int i = 0; i < MAX_DEVICES; i++){
        for(int j = 0; j < MAX_DEVICES; j++){
            if(devicespeeds_sorted[i] == devicespeeds[j]){              // Compare the index of the sorted device speed array
                strcpy(devicenames_sorted[i], devicenames[j]);          // Copy device name if the device speed index matches 
            }
        }
    }
}

// Global variables and arrays used for the simulation of job mix
#define SPACE 5                             // Used for the print output

int readyqueue[MAX_PROCESSES];              // Queue for processes that are ready to be run
int rqsize = 0;                             // Size of ready queue, used for indexing 
int blqueue[MAX_PROCESSES];                 // Blocked queue, waiting for slow i/o
int blqueuesize = 0;                        // Size of blocked queue, used for indexing 
int mblqueue[MAX_DEVICES][MAX_PROCESSES];   // Multi-blocked queue, waiting for slow i/o
int mblqueuesize[MAX_DEVICES] = {0};        // Sizes of each queue in multiblocked queue
int runningprocessindex = -1;               // When running process is -1, it means no processes are running
int time = 0;                               // Current time
int timeleft[MAX_PROCESSES] = {0};          // Array holding each process's remaining execution time
int iotimelefttostart[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS] = {{0}};   // Each io's remaining time until start
int ioruntimesleft[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int currenttq = 0;                          // Tracks the current time quantum 
bool databusfree = true;                    // Holds status of databus
int requestdatabusdelay = 5;                // To track the 5 second delay of the databus
int runningioprocess = -1;                  // The process which the currently running io belongs to 
int runningionumber = -1;                   // The 'ionumber' of the currently running io
int nexit = 0;                              // Number of processes which have exited

// Initialises all variables, used to reset vars when TQ increments
void initialisevariables(void){
    rqsize = 0;                             // Size of ready queue, used for indexing 
    blqueuesize = 0;                        // Size of blocked queue, used for indexing 
    runningprocessindex = -1;               // When running process is -1, it means no processes are running
    time = 0;                               // Current time
    currenttq = 0;                          // Tracks the current time quantum 
    databusfree = true;                     // Holds status of databus
    requestdatabusdelay = 5;                // To track the 5 second delay of the databus
    runningioprocess = -1;                  // The process which the currently running io belongs to 
    runningionumber = -1;                   // The 'ionumber' of the currently running io
    nexit = 0;                              // Number of processes which have exited
    
    for (int i =0; i<MAX_PROCESSES; i++) {                 // Initialise readyqueue and blqueue to empty (-1)
        readyqueue[i] = -1; 
        blqueue[i] = -1;
    }
    for (int i = 0; i < MAX_DEVICES; i++){                 // Initialise mblqueue to empty (-1)
        for(int j = 0; j < MAX_PROCESSES; j++){
            mblqueue[i][j] = -1;
        }
        mblqueuesize[i] = 0;
    }
    for(int i = 0; i < processcount; i++){
        timeleft[i] = processtimes[i][2];                  // Initialise timeleft to the process running time
    }
    memcpy(iotimelefttostart, iostart, sizeof (int) * MAX_PROCESSES * MAX_EVENTS_PER_PROCESS); // Copy iostart times into iotimeleft array
    memcpy(ioruntimesleft, ioruntimes, sizeof (int) * MAX_PROCESSES * MAX_EVENTS_PER_PROCESS); // Copy ioruntimes into ioruntimesleft array
}

// Prints readyqueue, running and nexit
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
    printf("]\t");
    printf("running=p%i   ", processtimes[runningprocessindex][0]);
    printf("nexit=%i\n", nexit);
}

// Moves ready queue forward
void qforward(void){
    for (int i = 0; i < MAX_PROCESSES-1; i++)
    {
        readyqueue[i] = readyqueue[i+1];        // Moves each element one index forward
    }
    readyqueue[MAX_PROCESSES-1] = -1;           // Sets end of queue to empty (-1)
    rqsize--;                                   // Decrease rqsize
    return;
}

// Moves blqueue forward
void mblqforward(int rownum){
    for (int i = 0; i < MAX_PROCESSES-1; i++)
    {
        mblqueue[rownum][i] = mblqueue[rownum][i+1];    // Move blqueue forward
    }
    mblqueue[rownum][MAX_PROCESSES-1] = -1;             // Sets end of queue to 0
    mblqueuesize[rownum]--;                             // Decrease size of mblqueue
    return;
}

// Checks if any new or previously blocked processes are ready
void checkready(void){
    for(int i = 0;i < processcount; i++){
        if(time == processtimes[i][1]){     // If a proccess time is equal to current time,
            readyqueue[rqsize] = i;         // Add the next process to readyqueue
            printf("time: %i\t p%i.NEW->READY", time, processtimes[i][0]); 
            printrq(SPACE+1);
            rqsize++;
        }
    }
}

// Requests use of databus if it free and there are any io queued 
void checkmblqueue(void){                           // MAYBE RENAME TO REQUEST DATABUS OR SMTH
    for(int i = 0; i < MAX_DEVICES; i++){
        if(databusfree && mblqueue[i][0] != -1){       
            databusfree = false;                        // Occupy databus
            runningioprocess = mblqueue[i][0];          // Set running io process number to start of blqueue
            mblqforward(i);                             // Move blqueue forward
            printf("time: %i\t p%i.request_databus", time, processtimes[runningioprocess][0]); 
            printrq(SPACE);

            for(int j = 0; i < MAX_EVENTS_PER_PROCESS; j++){
                if(iotimelefttostart[runningioprocess][j] == 0){    // Set running io number to io that
                    runningionumber = j;                            // is about to start (timeleft = 0)
                    break;                                          // Break once this is found to prevent overwriting the number
                }
            }
            break;      // Once an io is using the databus, stop checking
        }
    }
}

// Releases use of databus (used when an io has completed transferring)
void releasedatabus(void){
    printf("time: %i\t p%i.release_databus", time, processtimes[runningioprocess][0]); 
    printrq(SPACE);
    iotimelefttostart[runningioprocess][runningionumber] = -1;      // Sets the respective io index to -1 
    databusfree = true;                                             // Update databusfree variable to true
    requestdatabusdelay = 5;                                        // Reset the databus delay back to 5 seconds
    runningioprocess = -1;                                          // Reset running io process to -1 (no currently running io)
    runningionumber = -1;                                           // Reset running io number to -1 (no currently running io)
    checkmblqueue();                                                // Check if there are any processes waiting to use the databus
}

// Adds a process index to the ready queue
void addtorq(int process){
    readyqueue[rqsize] = process;                           // Add process to end of ready queue
    rqsize++;                                               // Increase ready queue size
}

// Adds a process index to the multi blocked queue in the relevant row
void addtomblq(int process, int row){
    mblqueue[row][mblqueuesize[row]] = process;             
    mblqueuesize[row]++;
}

// Updates times and io device times
void updatetime(void){
    time++; 
    if (runningprocessindex != -1) {                                // If there is a running process, tick relevant times down
        currenttq--;  
        timeleft[runningprocessindex]--;                            // Tick down current running process 
        for(int i = 0; i < MAX_EVENTS_PER_PROCESS; i++){
            if(iotimelefttostart[runningprocessindex][i] <= 0){     // If time is zero, don't tick down
                continue;
            }
            iotimelefttostart[runningprocessindex][i]--;            // Tick io times down for current process
        }
    }
    // i/o checks
    checkmblqueue();    // delete?
    if(databusfree == false){                                       // If databus is being used
        if(ioruntimesleft[runningioprocess][runningionumber] == 1 && requestdatabusdelay == 0){ // If i/o will complete in the next usec
            addtorq(runningioprocess);
            releasedatabus();
        }
        else if(requestdatabusdelay == 0){                          // If the 5 usec delay has passed
            ioruntimesleft[runningioprocess][runningionumber]--;    // Decrement time remaining for current i/o process 
        }
        else{
            requestdatabusdelay--;                                  // If 5 usec acquisition time hasnt passed, decrement this
        }
    }      
    checkready();
}

// Move process from ready to running if there is no currently running process
void checkrunning(void){
    if(runningprocessindex == -1 && readyqueue[0] != -1){
        for (int i = 0; i < 5; i++){
            updatetime();                                  // 5 usecs to change from READY->RUNNING
        }
        runningprocessindex = readyqueue[0];               // Set runningprocessindex to start of readyqueue
        qforward();                                        // Move ready queue forward
        printf("time: %i\t p%i.READY->RUNNING", time, processtimes[runningprocessindex][0]);
        printrq(SPACE);
    }
}

// Exits the currently running process
void exitprocess(void){
    nexit++;
    printf("time: %i\t p%i.RUNNING->EXIT", time, processtimes[runningprocessindex][0]);  
    runningprocessindex = -1;
    printrq(SPACE);  
}

// Checks if the currently running process has any remaining execution time
bool isfinished(void){ 
    if(timeleft[runningprocessindex] <=0){
        return true;
    }
    return false;
}

// Blocks currently running process (when requesting i/o)
void mblockprocess(int ionum) {
    for(int i = 0; i < MAX_DEVICES; i++){
        if(strcmp(iodevice[runningprocessindex][ionum], devicenames_sorted[i]) == 0){   // Checks which i/o device is being requested
            addtomblq(runningprocessindex, i);                                          // Add to the correct index of multi blocked queue
            break;                                                                      // Break once found
        }
    }
    printf("time: %i\t p%i.RUNNING->BLOCKED(%s)", time, processtimes[runningprocessindex][0],iodevice[runningprocessindex][ionum]);  
    runningprocessindex = -1;                     // Stops running process
    printrq(SPACE-1);
}

// Checks if running process needs any io
int checkio(void){
    for(int i = 0; i < MAX_EVENTS_PER_PROCESS; i++){
        if(iotimelefttostart[runningprocessindex][i] == 0){
            return i; // Return the ionumber if it needs io
        }
    }
    return -1;        // Return -1 if no io needed
}

// While loop for the job mix simulation
void job_mix_loop(int time_quantum){
    // Increment time if no processes running, and either databus is used or the next process's start time
    while((time < processtimes[nexit][1] || databusfree == false) && runningprocessindex == -1){     
        updatetime();
        checkrunning();
    }

    while (timeleft[runningprocessindex] > 0)               // Loop until process has no time remaining
    {
        currenttq = time_quantum;                           // The current TQ must be reset
        while(currenttq > 0)                                // Loop until end of current TQ
        {
            updatetime();       
            int ionum;
            if((ionum = checkio()) != -1){                  // If the process needs io,
                currenttq = time_quantum;                   // Reset time quantum
                mblockprocess(ionum);                       // Block the process
                checkmblqueue();                            // Check multi blocked queue for any processes
                checkrunning();                             // Start running the next process (if there is)
                return;                                     // Finish current job mix loop
            }
            if(isfinished()){                               // If process has terminated
                currenttq = time_quantum;                   // Reset time quantum
                exitprocess();                              // Exit the process
                return;                                     // Finish current job mix loop
            }
        }                                                   // Once this TQ is over
        if(readyqueue[0] != -1){                            // If there is a process waiting
            addtorq(runningprocessindex);                   // Add it to the readyqueue
            printf("time: %i\t p%i.expire,   p%i.RUNNING->READY", time, processtimes[runningprocessindex][0],processtimes[runningprocessindex][0]);
            runningprocessindex = -1;                       // Stop current process 
            printrq(SPACE-1);
            checkrunning();                                 // Check if the next process can run
        }
        else{                                               // Otherwise if there is no process waiting, refresh the TQ and continue execution
            printf("time: %i\t p%i.freshTQ", time, processtimes[runningprocessindex][0]); 
            printrq(SPACE+1);
        } 
    }
    exitprocess();
}

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
    initialisevariables();      // Initialise/reset variables
    currenttq = time_quantum;   // maybe delete
    printf("time: %i  \t reboot with TQ = %i\n", time, time_quantum);
    while(nexit < processcount){                                // While there are still processes to run
        job_mix_loop(time_quantum);
    }

    printf("running simulate_job_mix( time_quantum = %i usecs )\n",
                time_quantum);
    total_process_completion_time = time - processtimes[0][1];  // Completion time will be the final time minus the start time
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
    sortdevices();

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
//  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
//  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED

    int fastestcompletiontime = __INT_MAX__;
    for(int time_quantum=TQ0 ; time_quantum<=TQfinal ; time_quantum += TQinc) {
        simulate_job_mix(time_quantum);
        if(total_process_completion_time < fastestcompletiontime){
            optimal_time_quantum = time_quantum;
            fastestcompletiontime = total_process_completion_time;
        }
    }

//  PRINT THE PROGRAM'S RESULT
    printf("best %i %i\n", optimal_time_quantum, fastestcompletiontime);

    exit(EXIT_SUCCESS);
}