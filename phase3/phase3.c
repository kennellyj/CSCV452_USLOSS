#include <usloss.h>
#include <libuser.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
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
void terminate_real(int status);
static void gettimeofday(sysargs *args_ptr);
int gettimeofday_real();
static void cputime(sysargs *args_ptr);
int cputime_real();
static void getPID(sysargs *args_ptr);
int getPID_real();
static void semcreate(sysargs *args_ptr);
int semcreate_real(int semaphore);
static void semp(sysargs *args_ptr);
int  semp_real(int semaphore);
static void semv(sysargs *args_ptr);
int  semv_real(int semaphore);
static void semfree(sysargs *args_ptr);
int  semfree_real(int semaphore);

void check_kernel_mode(char *func_name);
void nullsys3(sysargs *args_ptr);

void init_procs(int i);
void init_sems(int i);
void addChildList(int parentID, int childID);
void rmChildList(int parentID, int childID);
int nextSems();

/****************************GLOBALS***********************************************/
mbox_proc phase3_ProcTable[MAXPROC];
semaphore SemTable[MAXSEMS];

int debugflag3 = 0;

int numSems = 0;
int nextSem = 0;

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
    sys_vec[SYS_GETPID] = getPID;
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
static void wait(sysargs *args_ptr) {

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
static void terminate(sysargs *args_ptr)
{
   // egt quit code from sys args
   int code = (long) args_ptr->arg1;

   // terminate with the given code
   terminate_real(code);

   // switch to user mode
   psr_set(psr_get() & ~PSR_CURRENT_MODE);
}
/* --------------------------------------------------------------------------------
        Name    - terminate_real
        Purpose - terminate called process and all of its children
        Parameters - status
        Return  - NONE
        Side Effects - Proc Status is set to UNUSED
   -------------------------------------------------------------------------------- */
void terminate_real(int status) {

//get the current process
mbox_proc cur_proc = phase3_ProcTable[getpid() % MAXPROC];

int result = -1;

if (!is_zapped()) {
   psr_set(psr_get() & ~PSR_CURRENT_MODE);

   int (*func)(char *) = cur_proc.start_func;
   char arg[MAXARG];
   strcpy(arg, cur_proc.start_arg);

   result = (func)(arg);

   terminate(result);
}
else {
   terminate_real(0);
}

return result;

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

long result = gettimeofday_real();
args_ptr->arg1 = (void *) result;
} /* GetTimeofDay */

int gettimeofday_real()
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

long result = cputime_real();
args_ptr->arg1 = (void *) result;

}

int cputime_real()
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

long result = getPID_real();
args_ptr->arg1 = (void *) result;
}

int getPID_real()
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
int  semp_real(int semaphore) {

} /* SemP */

/* --------------------------------------------------------------------------------
   Name - SemV
   Purpose - performs a "V" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid, 0 otherwise.
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
int  semv_real(int semaphore) {

} /* SemV */

/* --------------------------------------------------------------------------------
   Name - SemFree
   Purpose - frees a semaphore
   Parameters - semaphore handle
   Returns - arg4: -1 if semaphore handle is invalid.
                    1 if there are processes blocked on the semaphore, 0 otherwise.
   Side Effects -  terminate
   -------------------------------------------------------------------------------- */
int  semfree_real(int semaphore) {

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
        phase3_ProcTable[i].start_arg[0] = '\0';
        phase3_ProcTable[i].priority = -1;
        phase3_ProcTable[i].start_func = NULL;
        phase3_ProcTable[i].stack_size = -1;
        phase3_ProcTable[i].spawnbox = MboxCreate(1, MAXLINE);
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
   SemTable[i].mutex_box = -1;
   SemTable[i].value = 0;
   SemTable[i].blocked = 0;
   SemTable[i].blocked_box = -1;

   //add more as needed ********
}

/* --------------------------------------------------------------------------------
   Name - addChildList
   Purpose - adds child to list
   Parameters - parentID, child ID
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
void addChildList(int parentID, int childID)
{
   //set inputs to align with table
  parentID %= MAXPROC;
  childID %= MAXPROC;

   //increase the number of children
  phase3_ProcTable[parentID].numChild++;

   //if parent has no children, set child as parents child
  if (phase3_ProcTable[parentID].child_ptr == NULL) {
     phase3_ProcTable[parentID].child_ptr = &phase3_ProcTable[childID];
  }
  // if the parent DOES have chuildren, find last kid and appen child to last's next
  else {
     mbox_proc_ptr child;
     for (child = phase3_ProcTable[parentID].child_ptr; child->sibling_ptr != NULL;
          child = child->sibling_ptr) {}

      child->sibling_ptr = &phase3_ProcTable[childID];      
  }
}

/* --------------------------------------------------------------------------------
   Name - rmChildList
   Purpose - removes child from list
   Parameters - parentID, childID
   Returns - 
   Side Effects - 
-------------------------------------------------------------------------------- */
void rmChildList(int parentID, int childID)
{
      //set inputs that align with proc table
   int parentRE = parentID % MAXPROC;

   //increase number of children
   phase3_ProcTable[parentRE].numChild++;

   //find the child and ererence it if the child is parent's first child
   if (phase3_ProcTable[parentRE].child_ptr->pid == childID) {

      //swap the parent's first with the sibling
      phase3_ProcTable[parentRE].child_ptr = phase3_ProcTable[parentRE].child_ptr->sibling_ptr;

   }
   else{
      mbox_proc_ptr child;
      for (child = phase3_ProcTable[parentRE].child_ptr; child->sibling_ptr != NULL;
            child = child->sibling_ptr) {
               if (child->sibling_ptr->pid == childID) {
                  child->sibling_ptr = child->sibling_ptr->sibling_ptr;
                  break;
               }
            }

   }
}

/* --------------------------------------------------------------------------------
   Name - nextSems
   Purpose - obtains the next semaphore in the semaphore table
   Parameters - none
   Returns - 
   Side Effects - 
-------------------------------------------------------------------------------- */
int nextSems() {
   while (SemTable[nextSem].mutex_box != -1) {
      nextSem++;
      if (nextSem >= MAXSEMS) {
         nextSem = 0;
      }
   }
   return nextSem;
}