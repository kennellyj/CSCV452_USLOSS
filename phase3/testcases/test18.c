/* recursive terminate test */

#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int Child1(char *);
int Child2(char *);
int Child3(char *);

int sem1;

int start3(char *arg)
{
    int pid;
    int status;

    printf("start3(): started\n");
    Spawn("Child1", Child1, "Child1", USLOSS_MIN_STACK, 4, &pid);
    printf("start3(): spawned process %d\n", pid);
    Wait(&pid, &status);
    printf("start3(): child %d returned status of %d\n", pid, status);
    printf("start3(): done\n");
    Terminate(8);
    return 0;
} /* start3 */


int Child1(char *arg) 
{
    int pid;
    int status;

    GetPID(&pid);
    console("%s(): starting, pid = %d\n", arg, pid);
    Spawn("Child2", Child2, "Child2", USLOSS_MIN_STACK, 2, &pid);
    printf("%s(): spawned process %d\n", arg, pid);
    Wait(&pid, &status);
    printf("%s(): child %d returned status of %d\n", arg, pid, status);
    Spawn("Child3", Child3, "Child3", USLOSS_MIN_STACK, 5, &pid);
    printf("%s(): spawned process %d\n", arg, pid);
    Wait(&pid, &status);
    printf("%s(): child %d returned status of %d\n", arg, pid, status);
    console("%s(): done\n", arg);
    Terminate(9);

    return 0;
} /* Child1 */

int Child2(char *arg) 
{
    int pid;

    GetPID(&pid);
    console("%s(): starting, pid = %d\n", arg, pid);
    Spawn("Child2a", Child3, "Child2a", USLOSS_MIN_STACK, 5, &pid);
    printf("%s(): spawned process %d\n", arg, pid);
    Spawn("Child2b", Child3, "Child2b", USLOSS_MIN_STACK, 5, &pid);
    printf("%s(): spawned process %d\n", arg, pid);
    Spawn("Child2c", Child3, "Child2c", USLOSS_MIN_STACK, 5, &pid);
    printf("%s(): spawned process %d\n", arg, pid);
    Terminate(10);

    return 0;
} /* Child2 */

int Child3(char *arg) 
{
    console("%s(): starting\n", arg);
    Terminate(11);

    return 0;
} /* Child3 */
