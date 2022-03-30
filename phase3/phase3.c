#include <usloss.h>
#include <libuser.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include "sems.h"

/* ----------------------- PROTOTYPES ------------------------------------------ */

int start2(char *);
extern int start3(char *);

static void spawn(sysargs *args_ptr);
int spawn_real(char *name, int (*func)(char *), char *arg,
                int stack_size, int priority);
static int spawn_launch(char *arg);

static void wait(sysargs *args_ptr);
int wait_real(int *status);
static void terminate(sysargs *args_ptr);
void Terminate(int status);
static void gettimeofday(sysargs *args_ptr);
int GetTimeofDay();
static void cputime(sysargs *args_ptr);
int CPUTime();
static void getPID(sysargs *args_ptr);
int GetPID();
static void semcreate(sysargs *args_ptr);
int SemCreate(int semaphore);
static void semp(sysargs *args_ptr);
int  SemP(int semaphore);
static void semv(sysargs *args_ptr);
int  SemV(int semaphore);
static void semfree(sysargs *args_ptr);
int  SemFree(int semaphore);

void check_kernel_mode(char *func_name);
void nullsys3(sysargs *args_ptr);

void init_procs(int i);
void init_sems(int i);

/****************************GLOBALS***********************************************/
mbox_proc phase3_ProcTable[MAXPROC];
semaphore SemTable[MAXSEMS];

int debugflag3 = 0;

/* -------------------------- FUNCTIONS -------------------------------------------- */

start2(char *arg)
{
    int		pid;
    int		status;
    
    /* Check kernel mode */
    check_kernel_mode("start2");

    /* Data structure initialization */
    
    // Process Table
    for (int i = 0; i < MAXPROC; i++) {
        init_procs(i);
    }

    //Sys call handlers
    for (int i = 0; i < MAXSYSCALLS; i++) {
        sys_vec[i] = nullsys3;
    }
    sys_vec[SYS_SPAWN] = spawn;
    sys_vec[SYS_WAIT] = wait;
    sys_vec[SYS_TERMINATE] = Terminate;
    sys_vec[SYS_GETTIMEOFDAY] = GetTimeofDay;
    sys_vec[SYS_CPUTIME] = CPUTime;
    sys_vec[SYS_GETPID] = GetPid;
    sys_vec[SYS_SEMCREATE] = SemCreate;
    sys_vec[SYS_SEMP] = SemP;
    sys_vec[SYS_SEMV] = SemV;
    sys_vec[SYS_SEMFREE] = SemFree;

   // initiliaze the Semaphore Table
    for (int i = 0; i < MAXSEMS; i++)
    {
       init_sems(i);
    }


    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscall_handler; spawn_real is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes usyscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawn_real().
     *
     * Here, we only call spawn_real(), since we are already in kernel mode.
     *
     * spawn_real() will create the process by using a call to fork1 to
     * create a process executing the code in spawn_launch().  spawn_real()
     * and spawn_launch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawn_real() will
     * return to the original caller of Spawn, while spawn_launch() will
     * begin executing the function passed to Spawn. spawn_launch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawn_real() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
    pid = spawn_real("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    return 0;

} /* start2 */



/* --------------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks to make sure functions are being called in Kernel mode
   Parameters - *func_name keeps track of where the mode checking is being invoked.
   Returns - nothing
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
void check_kernel_mode(char *func_name) {
   char buffer[200];

   if (DEBUG3 && debugflag3) {
      sprintf(buffer, "check_kernel_mode(): called for function %s\n", func_name);
      console("%s", buffer);
   }

   /*test if in kernel mode and halts if in user mode*/
   if(psr_get() & PSR_CURRENT_MODE == 0) {
      console("%s", buffer);
      halt(1);
   }
   
} /* check_kernel_mode */

/* --------------------------------------------------------------------------------
   Name - nullsys3
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
static void nullsys3(sysargs *args_ptr) {
    printf("nullsys3(): Invalid syscall %d\n", args_ptr->number);
    printf("nullsys3(): Process %d terminating\n", getpid());
    terminate_real(1);
} /* nullsys3 */

/* --------------------------------------------------------------------------------
        Name    - spawn
        Purpose - creates a user-level process
        Parameters - 
        Return  - arg1: PID of newly created process; -1 if not created
                  arg4: -1 if illegalvalues are given as input; 0 otherwise.
        Side Effects - spawn_real is called
   -------------------------------------------------------------------------------- */
static void spawn(sysargs *args_ptr) {

int kid_pid;
char *arg;
int (*func)(char *);
int stack_size;
int priority;
char* name;

if (is_zapped())
{
   Terminate(0);

}

func = args_ptr->arg1;
arg = args_ptr->arg2;
stack_size = (int) args_ptr->arg3;
priority = (int) args_ptr->arg4;
name = (char*) args_ptr->arg5;
// more code to extract system call arguments as well as exceptional handling

//call another function to modularize the code better
kid_pid = spawn_real(name, func, arg, stack_size, priority);

args_ptr->arg1 = (void *) kid_pid; //packing to return back to the caller
args_ptr->arg4 = (void *) 0;

if (is_zapped()) 
{
   Terminate(0);
}

/* set to user mode */  // more code to write
psr_set(psr_get() & ~PSR_CURRENT_MODE);
return ;

} /* spawn */

/* --------------------------------------------------------------------------------
   Name - spawn_real
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority) {

int kidpid;
int my_location; /* parent's location in proc table */
int kid_location; /* child's location in proc table */
int result;
//u_proc_ptr kidptr, prevptr;


my_location = getpid() % MAXPROC;

/* create our child */

kidpid = fork1(name, spawn_launch, NULL, stack_size, priority);
//more to check the kidpid and put the new process data to the process table

//Then synchronize with the child using a mailbox

result = MboxSend(phase3_ProcTable[kid_location].mbox_start, &my_location, sizeof(int));

//more to add

return kidpid;

} /* spawn_real */

/* --------------------------------------------------------------------------------
   Name - spawn_launch
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
static int spawn_launch(char *arg)
{
   int parent_location = 0;
   int my_location;
   int result;
   int (* start_func)(char *);
   char *start_arg;
   //more to add if you see necessary

   my_location = getpid() % MAXPROC;

   /* Sanity CHeck */

   /*Maintain the process table entry, you can add more */
   phase3_ProcTable[my_location].status = ACTIVE;

   //sybnchronize with the parent here,
   //which function to call?

   //then get the start function and its argument

   if (!is_zapped())
   {
      //more code if you see necessary 
      //set up user mode
      psr_set(psr_get() & ~PSR_CURRENT_MODE);
      result = (start_func)(start_arg);
      Terminate(result);
   }
   else{
      terminate_real(0);
   }

   printf("spawn_launch(): should not see this message following Terminate!\n");
   return 0;

} /* spawn_launch */
/* --------------------------------------------------------------------------------
        Name    - wait
        Purpose - waits for child process to terminate
        Parameters - pid, status
        Return  - arg1: process ID of terminating child
                  arg2: the termination code of the child
                  arg4: -1 if the process has no children, 0 otherwise
        Side Effects - process is blocked if no terminating children
   -------------------------------------------------------------------------------- */
int Wait(int *pid, int *status) {

} /* Wait */

/* --------------------------------------------------------------------------------
   Name - 
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
int wait_real(int *status) {

} /* wait_real */

/* --------------------------------------------------------------------------------
        Name    - terminate
        Purpose - terminate called process and all of its children
        Parameters - status
        Return  - NONE
        Side Effects - Proc Status is set to UNUSED
   -------------------------------------------------------------------------------- */
void Terminate(int status) {

} /* Terminate */

/* --------------------------------------------------------------------------------
   Name - GetTimeofDay
   Purpose - Creturns the value of the time-of-day clock
   Parameters - tod
   Returns - time of day
   Side Effects - 
   -------------------------------------------------------------------------------- */
static void gettimeofday(sysargs *args_ptr) {
if (debugflag3)
{
   console("process %d: GetTimeofDay\n", getpid());
}

long result = GetTimeofDay();
args_ptr->arg1 = (void *) result;
} /* GetTimeofDay */

int GetTimeofDay(int *tod)
{
   if (debugflag3)
   {
      console("Process %d: GetTimeofDay\n", getpid());
   }
   return clock();
}
/* --------------------------------------------------------------------------------
   Name - CPUTime
   Purpose - returns the CPU time of the process
   Parameters - cpu
   Returns - the process' CPU time
   Side Effects -  
   -------------------------------------------------------------------------------- */
static void cputime(sysargs *args_ptr) {
if (debugflag3)
{
   console("process %d: CPUTime\n", getpid());
}

long result = CPUTime();
args_ptr->arg1 = (void *) result;

}

int CPUTime(int *cpu)
{
   if (debugflag3)
   {
      console("Process %d: CPUTime\n", getpid());
   }
   return readtime(); 

} /* CPUTime */

/* --------------------------------------------------------------------------------
   Name - GetPID
   Purpose - returns the process ID of the currently running process.
   Parameters - pid
   Returns - Process ID
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
static void getPID(sysargs *args_ptr) 
{
if (debugflag3)
{
   console("process %d: GetPid\n", getpid());
}

long result = GetPid();
args_ptr->arg1 = (void *) result;
}

int GetPid()
{
   if (debugflag3)
   {
      console("Process %d: CPUTime\n", getpid());
   }
   return getpid(); 
} /* GetPID */

/* --------------------------------------------------------------------------------
   Name - SemCreate
   Purpose - creates a user level semaphore
   Parameters - value, *semaphore
   Returns - arg1: semaphore handle to be used in subsequent semaphore system calls
             arg4: -1 if initial value is negative or no more semaphores are 
                    available, 0 otherwise
   Side Effects -  
   -------------------------------------------------------------------------------- */
static void semcreate(sysargs *args_ptr) {

} /* SemCreate */

/* --------------------------------------------------------------------------------
   Name - SemP
   Purpose - performs a "P" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid; 0 otherwise.
   Side Effects -  
   -------------------------------------------------------------------------------- */
int  SemP(int semaphore) {

} /* SemP */

/* --------------------------------------------------------------------------------
   Name - SemV
   Purpose - performs a "V" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid, 0 otherwise.
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
int  SemV(int semaphore) {

} /* SemV */

/* --------------------------------------------------------------------------------
   Name - SemFree
   Purpose - frees a semaphore
   Parameters - semaphore handle
   Returns - arg4: -1 if semaphore handle is invalid.
                    1 if there are processes blocked on the semaphore, 0 otherwise.
   Side Effects -  terminate
   -------------------------------------------------------------------------------- */
int  SemFree(int semaphore) {

} /* SemFree */

/* --------------------------------------------------------------------------------
   Name - init_proc
   Purpose - initializes the process at any given index throughout table
   Parameters - i
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
   void init_procs(int i)
   {
        phase3_ProcTable[i].pid = -1;
        phase3_ProcTable[i].status = NULL;
        phase3_ProcTable[i].name = NULL;
        phase3_ProcTable[i].numChild = -1;
        phase3_ProcTable[i].parent_ptr = NULL;
        phase3_ProcTable[i].child_ptr = NULL;
        phase3_ProcTable[i].sibling_ptr = NULL;

        //add more as needed ********

   } /* init_procs */

/* --------------------------------------------------------------------------------
   Name - init_sems
   Purpose - Initialize the Semaphore table for a given index
   Parameters - i
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
void init_sems(int i)
{
   SemTable[i].status = NULL;
   SemTable[i].sid = i;

   //add more as needed ********
}