#include <usloss.h>
#include <libuser.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <stdlib.h>
#include <string.h>
#include "sems.h"
#include <time.h>
#include <stdio.h>

/* ----------------------- PROTOTYPES ------------------------------------------ */

int start2(char *arg);
extern int start3(char *);

static void spawn(sysargs *args_ptr);
int spawn_real(char *name, int (*func)(char *), char *arg,
                int stack_size, int priority);
static int spawn_launch(char *arg);

static void wait(sysargs *args_ptr);
int wait_real(long *status);
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
static void nullsys3(sysargs *args_ptr);

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

int numProcs = 3;

/* -------------------------- FUNCTIONS -------------------------------------------- */
int start2(char *arg)
{
    int		pid;
    int status;
    
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
    sys_vec[SYS_TERMINATE] = terminate;
    sys_vec[SYS_GETTIMEOFDAY] = gettimeofday;
    sys_vec[SYS_CPUTIME] = cputime;
    sys_vec[SYS_GETPID] = getPID;
    sys_vec[SYS_SEMCREATE] = semcreate;
    sys_vec[SYS_SEMP] = semp;
    sys_vec[SYS_SEMV] = semv;
    sys_vec[SYS_SEMFREE] = semfree;

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

    if (pid > 0) {
       pid = wait_real(&status);
    }
    

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
   if((psr_get() & PSR_CURRENT_MODE) == 0) {
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

numProcs++;


if (is_zapped())
{
   Terminate(0);

}

if (numProcs > MAXPROC) {
   args_ptr->arg4 = (void *) -1L;

   numProcs--;
   return;
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
/* child's location in proc table */
//u_proc_ptr kidptr, prevptr;

/* create our child */

kidpid = fork1(name, spawn_launch, NULL, stack_size, priority);

//more to check the kidpid and put the new process data to the process table
my_location = kidpid % MAXPROC;

phase3_ProcTable[my_location].pid = kidpid;
strcpy(phase3_ProcTable[my_location].name, name);
if (arg != NULL) {
   strcpy(phase3_ProcTable[my_location].start_arg, arg);
}
phase3_ProcTable[my_location].priority = priority;
phase3_ProcTable[my_location].start_func = func;
phase3_ProcTable[my_location].stack_size = stack_size;
phase3_ProcTable[my_location].parent_pid = getpid();

addChildList(getpid(), kidpid);

//Then synchronize with the child using a mailbox
MboxSend(phase3_ProcTable[my_location].spawnbox, NULL, 0);

//result = MboxSend(phase3_ProcTable[kid_location].mbox_start, &my_location, sizeof(int));

//more to add


if (is_zapped()) {
   terminate_real(0);
}

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

   int result;

   //more to add if you see necessary

   MboxReceive(phase3_ProcTable[getpid() % MAXPROC].spawnbox, NULL, 0);

   mbox_proc cur_proc = phase3_ProcTable[getpid() % MAXPROC];

   int my_location = getpid() % MAXPROC;

   /* Sanity CHeck */
   result = -1;

   //synchronize with the parent here,
   //which function to call?

   //then get the start function and its argument

   if (!is_zapped())
   {
      //set up user mode
      psr_set(psr_get() & ~PSR_CURRENT_MODE);

      //more code if you see necessary 
      int (*func)(char *) = cur_proc.start_func;
      char arg[MAXARG];
      strcpy(arg, cur_proc.start_arg);


      result = (func)(arg);

      Terminate(result);
   }
   else{
      terminate_real(0);
   }

   printf("spawn_launch(): should not see this message following Terminate!\n");
   return result;

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
   if (debugflag3) {
      console("Process %d: wait\n", getpid());

   }

   long code;

   long result = wait_real(&code);

   long success = 0;

   if (result == -2) {
      success = -1;
   }

   args_ptr->arg1 = (void *) result;
   args_ptr->arg2 = (void *) code;
   args_ptr->arg4 = (void *) success;

   psr_set(psr_get() & ~PSR_CURRENT_MODE);


} /* Wait */

/* --------------------------------------------------------------------------------
   Name - wait_real
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
int wait_real(long *status) {
   if (debugflag3) {
      console("Process %d: wait_real\n", getpid());
   }

   int result = join(status);

   if (is_zapped()) {
      terminate_real(0);
   }

   return result;

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

   if (debugflag3) {
      console("process %d: terminate\n", getpid());
   }

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

// if it has children, get rid oif children
if (cur_proc.numChild != 0) {
   int children[MAXPROC];
   int i = 0;
   for (mbox_proc_ptr child = cur_proc.child_ptr; child != NULL; child = child->sibling_ptr) {
      children[i] = child->pid;
      i++;
   }

    //go through all the children and zap
   for (int i = 0; i < cur_proc.numChild; i++) {
      zap(children[i]);
   }
}

rmChildList(cur_proc.parent_pid, cur_proc.pid);

int my_location = cur_proc.pid % MAXPROC;
phase3_ProcTable[my_location].pid = -1;
phase3_ProcTable[my_location].parent_pid = -1;
phase3_ProcTable[my_location].name = NULL;
phase3_ProcTable[my_location].numChild = -1;
phase3_ProcTable[my_location].parent_ptr = NULL;
phase3_ProcTable[my_location].child_ptr = NULL;
phase3_ProcTable[my_location].sibling_ptr = NULL;
phase3_ProcTable[my_location].start_arg[0] = '\0';
phase3_ProcTable[my_location].priority = -1;
phase3_ProcTable[my_location].start_func = NULL;
phase3_ProcTable[my_location].stack_size = -1;
phase3_ProcTable[my_location].spawnbox = MboxCreate(1, MAXLINE);

quit(status);

numProcs--;


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
   if (debugflag3) {
      console("Process %d: semcreate\n", getpid());
   }

   long address = semcreate_real((long) args_ptr->arg1);

   if (address == -1) {
      args_ptr->arg4 = (void *) -1L;
      args_ptr->arg1 = NULL;
   } else {
      args_ptr->arg4 = 0;
      args_ptr->arg1 = (void *) address;
   }
} /* SemCreate */

/* --------------------------------------------------------------------------------
   Name - SemCreate_real
   Purpose - creates a user level semaphore
   Parameters - value, *semaphore
   Returns - arg1: semaphore handle to be used in subsequent semaphore system calls
             arg4: -1 if initial value is negative or no more semaphores are 
                    available, 0 otherwise
   Side Effects -  
   -------------------------------------------------------------------------------- */
int semcreate_real(int semaphore) {
   if (debugflag3) {
      console("process %d: semcreate_real\n", getpid());
   
   }

   if (numSems >= MAXSEMS) {
      return -1;
   }

   int mutex_box = MboxCreate(1, 0);

   if (mutex_box == -1) {
      return -1;
   }

   int blocked_box = MboxCreate(0,0);

   if (blocked_box == -1) {
      return -1;
   }

   numSems++;
   int sem = nextSems();
   SemTable[semaphore].mutex_box = mutex_box;
   SemTable[semaphore].blocked_box = blocked_box;
   SemTable[semaphore].value = semaphore;
   SemTable[semaphore].blocked = 0;

   return sem;

} /* SemCreate */


/* --------------------------------------------------------------------------------
   Name - SemP_real
   Purpose - performs a "P" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid; 0 otherwise.
   Side Effects -  
   -------------------------------------------------------------------------------- */
int  semp_real(int semaphore) {
   if (debugflag3) {
      console("process %d: semp_real\n", getpid());
      
   }

   if (SemTable[semaphore].mutex_box == -1) {
      return -1;
   }

   int mutex_box = SemTable[semaphore].mutex_box;
   int blocked_box = SemTable[semaphore].blocked_box;

   MboxSend(mutex_box, NULL, 0);
   if (debugflag3) {
      console("Process %d: in critcal section\n", getpid());
   }

   int broke = 0;

   while (SemTable[semaphore].value <= 0) {
      SemTable[semaphore].blocked++;

      MboxReceive(mutex_box, NULL, 0);

      if (debugflag3) {
         console("Process %d: Blocking on p\n", getpid());
      }

      MboxSend(blocked_box, NULL, 0);

      if (is_zapped()) {
         terminate_real(0);
      }

      if (SemTable[semaphore].mutex_box == -1) {
         broke = 1;
         break;
      }

      if (debugflag3) {
         console("Process %d: Unblocked on P\n", getpid());
      }

      MboxSend(mutex_box, NULL, 0);
      if (debugflag3) {
         console("Process %d: in critical section\n", getpid());
      }
   }

   if (!broke) {
      SemTable[semaphore].value--;

      MboxReceive(mutex_box, NULL, 0);
   } else {
      terminate_real(1);
   }

   return 0;


} /* SemP_real */

/* --------------------------------------------------------------------------------
   Name - Semp
   Purpose - 
   Parameters - args_ptr
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
static void semp(sysargs *args_ptr) {
   if (debugflag3) {
      console("Process %d: semp\n", getpid());
   }

   int sem_id = (long) args_ptr->arg1;

   long result = semp_real(sem_id);

   args_ptr->arg4 = (void *) result;

} /* semp */

/* --------------------------------------------------------------------------------
   Name - Semv_real
   Purpose - performs a "V" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid, 0 otherwise.
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
int  semv_real(int semaphore) {

   if (SemTable[semaphore].mutex_box == -1) {
      return -1;
   }

   int mutex_box = SemTable[semaphore].mutex_box;
   int blocked_box = SemTable[semaphore].blocked_box;

   MboxSend(mutex_box, NULL, 0);

   SemTable[semaphore].value++;

   if (SemTable[semaphore].blocked > 0) {
      if (debugflag3) {
         console("process %d: freeing process blocked on P\n", getpid());
      }
      MboxReceive(blocked_box, NULL, 0);
      SemTable[semaphore].blocked--;
   }

   MboxReceive(mutex_box, NULL, 0);

   if (is_zapped()) {
      terminate_real(0);
   }

   return 0;

} /* Semv_real*/

/* --------------------------------------------------------------------------------
   Name - Semv
   Purpose - performs a "V" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid, 0 otherwise.
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
static void semv(sysargs *args_ptr) {
   if (debugflag3) {
      console("process %d: semv\n", getpid());
   }

   int sem_id = (long) args_ptr->arg1;

   long result = semv_real(sem_id);

   args_ptr->arg4 = (void *) result;

} /* semv */

/* --------------------------------------------------------------------------------
   Name - SemFree_real
   Purpose - frees a semaphore
   Parameters - semaphore handle
   Returns - arg4: -1 if semaphore handle is invalid.
                    1 if there are processes blocked on the semaphore, 0 otherwise.
   Side Effects -  terminate
   -------------------------------------------------------------------------------- */
int  semfree_real(int semaphore) {

   if (SemTable[semaphore].mutex_box == -1) {
      return -1;
   }

   SemTable[semaphore].mutex_box = -1;


   int result = 0;

   if (SemTable[semaphore].blocked > 0) {
      result = 1;

      for (int i = 0; i < SemTable[semaphore].blocked; i++) {
         MboxReceive(SemTable[semaphore].blocked_box, NULL, 0);
      }
   }

   SemTable[semaphore].blocked_box = -1;
   SemTable[semaphore].value = -1;
   SemTable[semaphore].blocked = 0;

   MboxRelease(SemTable[semaphore].mutex_box);
   MboxRelease(SemTable[semaphore].blocked_box);

   if (is_zapped()) {
      terminate_real(0);

   }

   numSems--;

   return result;

} /* SemFree_real */

/* --------------------------------------------------------------------------------
   Name - Semfree
   Purpose - frees a semaphore
   Parameters - semaphore handle
   Returns - arg4: -1 if semaphore handle is invalid.
                    1 if there are processes blocked on the semaphore, 0 otherwise.
   Side Effects -  terminate
   -------------------------------------------------------------------------------- */
static void semfree(sysargs *args_ptr) {
   if (debugflag3) {
      console("Process %d: semfree\n", getpid());
   }

   int semaphore = (long) args_ptr->arg1;

   if (semaphore == -1) {
      args_ptr->arg4 = (void *) -1L;
   
   } else {
      long result = semfree_real(semaphore);
      args_ptr->arg4 = (void *) result;
   }
} /* semfree */

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