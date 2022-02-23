/* ------------------------------------------------------------------------
   phase2.c
   Justin Kennelly & Carlos Torres
   Applied Technology
   College of Applied Science and Technology
   The University of Arizona
   CSCV 452

   ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>

#include "message.h"

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);
check_kernel_mode(char *func_name);
enableInterrupts();
disableInterupts();

int MboxCreate(int slots, int slot_size);
int MboxSend(int mbox_id, void *msg_ptr, int msg_size);
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size);

int MboxRelease(int mbox_id);
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size);
int MboxCondReceive(int mbox_id, void *msg_ptr, int max_msg_size);

int waitdevice(int type, int unit, int *status);


/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];
mail_slot MailBoxSlots[MAXSLOTS];
mbox_proc Phase2_ProcTable[MAXPROC];

/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg)
{
   int kid_pid, status; 

   if (DEBUG2 && debugflag2)
      console("start1(): at beginning\n");

   check_kernel_mode("start1");

   /* Disable interrupts */
   disableInterrupts();

   /* Initialize the mail box table, slots, & other data structures.
    * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
    * handlers.  Etc... */

   // Initialize the process table for phase 2
   init_Table(PROC_TABLE);

   //initialise mailboxtable
   init_Table(MAILBOX_TABLE);

   //initialise mailbox slots
   init_Table(SLOT_TABLE);

   enableInterrupts();

   /* Create a process for start2, then block on a join until start2 quits */
   if (DEBUG2 && debugflag2)
      console("start1(): fork'ing start2 process\n");
   kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);
   if ( join(&status) != kid_pid ) {
      console("start2(): join returned something other than start2's pid\n");
   }

   return 0;
} /* start1 */


/* ------------------------------------------------------------------------
   Name - MboxCreate
   Purpose - gets a free mailbox from the table of mailboxes and initializes it 
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   Side Effects - initializes one element of the mail box array. 
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size) {
   /* test if in kernel mode & disable interupts */
   check_kernel_mode("MboxCreate");
   disableInterrupts(); 



} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
   /* test if in kernel mode & disable interupts */
   check_kernel_mode("MboxSend");
   disableInterrupts(); 


} /* MboxSend */


/* ------------------------------------------------------------------------
   Name - MboxReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             Block the receiving process if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size) {
   /* test if in kernel mode & disable interupts */
   check_kernel_mode("MboxReceive");
   disableInterrupts(); 



} /* MboxReceive */


int check_io(){

    return 0;

} /* check_io */

<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FUNCTIONS ADDED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/* --------------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks to make sure functions are being called in Kernel mode
   Parameters - *func_name keeps track of where the mode checking is being invoked.
   Returns - nothing
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
static void check_kernel_mode(char *func_name) {
   union psr_values caller_psr;  /*Holds the caller's PSR values*/
   char buffer[200];

   if (DEBUG && debugflag) {
      sprintf(buffer, "check_kernel_mode(): called for function %s\n", func_name);
      console("%s", buffer);
   }

   /*test if in kernel mode and halts if in user mode*/
   caller_psr.integer_part = psr_get();
   if(caller_psr.bits.cur_mode == 0) {
      sprintf(buffer, "%s(): called while in user mode, process %d. Halting...\n", func_name, Current->pid);
      console("%s", buffer);
      halt(1);
   }
   
} /* check_kernel_mode */

/* ------------------------------------------------------------------------
   Name - disableInterrupts
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects -  
   ----------------------------------------------------------------------- */
void disableInterrupts()
{
  /* turn the interrupts OFF iff we are in kernel mode */
   if((PSR_CURRENT_MODE & psr_get()) == 0) {
      //not in kernel mode
      console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
      halt(1);
   } 
   else {
      /* We ARE in kernel mode, disable interrupts */
      psr_set( psr_get() & ~PSR_CURRENT_INT );
   }
} /* disableInterrupts */

/* --------------------------------------------------------------------------------
   Name - enableInterupts
   Purpose - Turn the interupts on
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
void enableInterrupts() {
   /* turn the interrupts OFF iff we are in kernel mode */
   if((PSR_CURRENT_MODE & psr_get()) == 0) {
      //not in kernel mode
      console("Kernel Error: Not in kernel mode, may not enable interrupts\n");
      halt(1);
   } 
   else {
      /* We ARE in kernel mode, enable interrupts */
      psr_set( psr_get() | PSR_CURRENT_INT );
   }
} /* enableInterupts */

/* --------------------------------------------------------------------------------
   Name - init_Table
   Purpose - 
   Parameters - 
   Returns - 
   Side Effects - 
   -------------------------------------------------------------------------------- */
void init_Table(int type) {
   int i = 0;

   switch(type) {
      case 'MAILBOX_TABLE':
         for (i; i < MAXMBOX; i++) {
            MailBoxTable[i].mbox_id = -1;
            MailBoxTable[i].status = UNUSED;
            MailBoxTable[i].num_slots = -1;
            MailBoxTable[i].max_slot_size = -1;
            /* MailBoxTable[i]. */
            /* MailBoxTable[i]. */
            /* MailBoxTable[i]. */
         }
         break;
      case 'SLOT_TABLE':
         for (i; i < MAXSLOTS; i++) {
            MailBoxSlots[i].mbox_id = -1;
            MailBoxSlots[i].status = UNUSED;
            MailBoxSlots[i].message = NULL;
            MailBoxSlots[i].next_slot = NULL;
         }
         break;
      case 'PROC_TABLE':
         for (i; i < MAXPROC; i++) {
            Phase2_ProcTable[i].pid = -1;
            Phase2_ProcTable[i].next_ptr = NULL;
         }
         break;
      default:
         console("ERROR: table type was not found\n");
         break;
      }

} /* init_Table */
