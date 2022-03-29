#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <libuser.h>
#include <sems.h>
#include <string.h>

/* ------------------------ PROTOTYPES ---------------------------------------------- */

int start2(char *); 
extern int start3(char *);
int  spawn_real(char *name, int (*func)(char *), char *arg,
                int stack_size, int priority);
int  wait_real(int *status);
void Terminate(int status);

int wait(int *pid, int *status);
void Terminate(int status);
void GetTimeofDay(int *tod);
void CPUTime(int *cpu);
void GetPID(int *pid);

int  SemCreate(int value, int *semaphore);
int  SemP(int semaphore);
int  SemV(int semaphore);
int  SemFree(int semaphore);


void check_kernel_mode(char *func_name);



/* ------------------------ GLOBALS ------------------------------------------------- */
Phase3ProcTable procTable[MAXPROC];


/* ------------------------ FUNCTIONS ----------------------------------------------- */


/* ----------------------------------------------------------------------------------
        Name    - start2
        Purpose - initilize the process table, semaphore table, and sys_call vec
        Parameters - arg: function arguments
        Return  - 0
        Side Effects - initializes all phase 3 structs
    --------------------------------------------------------------------------------- */
start2(char *arg)
{
    int		pid;
    int		status;
    /*
     * Check kernel mode here.
     */

    check_kernel_mode("start2");

    /*
     * Data structure initialization as needed...
     */

    for (int i = 0; i < MAXPROC; i++)
    {
        procTable[i].status = UNUSED;
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

} /* start2 */

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FUNCTIONS ADDED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/* ----------------------------------------------------------------------------------
        Name    - spawn
        Purpose - creates a user-level process
        Parameters - sysargs
        Return  - arg1: PID of newly created process; -1 if not created
                  arg4: -1 if illegalvalues are given as input; 0 otherwise.
        Side Effects - spawn_real is called
    --------------------------------------------------------------------------------- */
void spawn (sysargs *args)
{

} /* spawn */

/* ----------------------------------------------------------------------------------
        Name    - wait
        Purpose - waits for child process to terminate
        Parameters - pid, status
        Return  - arg1: process ID of terminating child
                  arg2: the termination code of the child
                  arg4: -1 if the process has no children, 0 otherwise
        Side Effects - process is blocked if no terminating children
    --------------------------------------------------------------------------------- */
int wait(int *pid, int *status)
{

}/* wait */

/* ----------------------------------------------------------------------------------
        Name    - terminate
        Purpose - terminate called process and all of its children
        Parameters - status
        Return  - NONE
        Side Effects - Proc Status is set to UNUSED
    --------------------------------------------------------------------------------- */
void Terminate(int status)
{

} /* Terminate */

/* --------------------------------------------------------------------------------
   Name - SemCreate
   Purpose - creates a user level semaphore
   Parameters - value, *semaphore
   Returns - arg1: semaphore handle to be used in subsequent semaphore system calls
             arg4: -1 if initial value is negative or no more semaphores are 
                    available, 0 otherwise
   Side Effects -  
   -------------------------------------------------------------------------------- */
int SemCreate(int value, int *semaphore)
{

} /* SemCreate */
 /* --------------------------------------------------------------------------------
   Name - SemP
   Purpose - performs a "P" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid; 0 otherwise.
   Side Effects -  
   -------------------------------------------------------------------------------- */  
int SemP(int semaphore)
{

} /* SemP */
/* --------------------------------------------------------------------------------
   Name - SemV
   Purpose - performs a "V" operation on a semaphore
   Parameters - semaphore
   Returns - arg4: -1 if semaphore handle is invalid, 0 otherwise.
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
int SemV(int semaphore)
{

} /* SemV */
/* --------------------------------------------------------------------------------
   Name - SemFree
   Purpose - frees a semaphore
   Parameters - semaphore handle
   Returns - arg4: -1 if semaphore handle is invalid.
                    1 if there are processes blocked on the semaphore, 0 otherwise.
   Side Effects -  terminate
   -------------------------------------------------------------------------------- */
int SemFree(int semaphore)
{

} /* SemFree */

/* --------------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks to make sure functions are being called in Kernel mode
   Parameters - *func_name keeps track of where the mode checking is being invoked.
   Returns - nothing
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
void check_kernel_mode(char *func_name) {
    char buffer[200];

   if ((PSR_CURRENT_MODE & psr_get()) == 0) {
      sprintf(buffer, "check_kernel_mode(): called for function %s\n", func_name);
      console("%s", buffer);
      halt(1);
   }
   
} /* check_kernel_mode */

/* --------------------------------------------------------------------------------
   Name - GetTimeofDay
   Purpose - Creturns the value of the time-of-day clock
   Parameters - tod
   Returns - time of day
   Side Effects - 
   -------------------------------------------------------------------------------- */
void GetTimeofDay(int *tod)
{

} /* GetTimeofDay */

/* --------------------------------------------------------------------------------
   Name - CPUTime
   Purpose - returns the CPU time of the process
   Parameters - cpu
   Returns - the process' CPU time
   Side Effects -  
   -------------------------------------------------------------------------------- */
void CPUTime(int *cpu)
{

} /* CPUTime */

/* --------------------------------------------------------------------------------
   Name - GetPID
   Purpose - returns the process ID of the currently running process.
   Parameters - pid
   Returns - Process ID
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
void GetPID(int *pid)
{

} /* GetPID */