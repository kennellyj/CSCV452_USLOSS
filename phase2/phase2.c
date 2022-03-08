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
#include <stdio.h>

#include "message.h"

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *); 

int MboxCreate(int slots, int slot_size);
int MboxSend(int mbox_id, void *msg_ptr, int msg_size);
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size);

int MboxRelease(int mbox_id);
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size);
int MboxCondReceive(int mbox_id, void *msg_ptr, int max_msg_size);

void check_kernel_mode(char *func_name);
void enableInterrupts();
void disableInterrupts();
void clock_handler2(int dev, void *punit);
void disk_handler(int dev, void *punit);
void terminal_handler(int dev, void *punit);
void syscall_handler(int dev, void *unit);
void nullsys(sysargs *args); 

void zero_mbox(int mbox_id);
void zero_slot(int slot_id);
void zero_mbox_slot(int pid);

int get_slot_index();
slot_ptr init_slot(int slot_index, int mbox_id, void *msg_ptr, int msg_size);
int add_slot_list(slot_ptr added_slot, mboxPtr mbox_ptr);

int check_io();
int waitdevice(int type, int unit, int *status);


/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];
mail_slot MailBoxSlots[MAXSLOTS];
/* Phase 2 Process Table */
mbox_proc Phase2_ProcTable[MAXPROC];

// For keeping track of the number of mailboxes used
int mbox_slots_used = 0;

// Define interrupt vector 
//void(*int_vec[NUM_INTS])(int dev, void *arg);
// Define system call vector
void(*sys_vec[MAXSYSCALLS])(sysargs *args);

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
   // loop index
   int i;

   if (DEBUG2 && debugflag2)
      console("start1(): at beginning\n");

   check_kernel_mode("start1");

   /* Disable interrupts */
   disableInterrupts();

   /* Initialize the mail box table, slots, & other data structures.
    * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
    * handlers.  Etc... */

   // Initialize the process table for phase 2
   
   for (i = 0; i < MAXPROC; i++) {
   	Phase2_ProcTable[i];
      zero_mbox_slot(i);
   }

   //initialise mailboxtable
   for (i = 0; i < MAXMBOX; i++) {
     	MailBoxTable[i].mbox_id = i;
      zero_mbox(i);
   }

   //initialise mailbox slots
   for (i = 0; i < MAXSLOTS; i++) {
      MailBoxSlots[i].slot_id = i;
      zero_slot(i);
   }

   /* Initialize clock mailboxe */
   for (i = 0; i < 7; i++) {
      MboxCreate(0, MAX_MESSAGE);
   }

   int_vec[CLOCK_DEV] = clock_handler2;
   int_vec[DISK_DEV] = disk_handler;
   int_vec[TERM_DEV] = terminal_handler;

   int_vec[SYSCALL_INT] = syscall_handler;

   for (i = 0; i < MAXSYSCALLS; i++) {
      sys_vec[i] = nullsys;
   }
   
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
   
   int m_box_id;

   /* Check if the slot size is within range*/
   if ((slot_size < 0) || (slot_size > MAX_MESSAGE)) {
      if (DEBUG2 && debugflag2) {
         console("Slot size is not valid.\n");
      }
      return -1;    
   }
   if ((slots < 0) || (slots > MAXSLOTS)) {
      if (DEBUG2 && debugflag2) {
         console("Slots are not valid.\n");
      }
      return -1;         
   }
   // Check if there are available slots
   if (mbox_slots_used >= MAXMBOX) {
   	if (DEBUG2 && debugflag2) {
         console("MboxCreate(): No mailboxes are available\n");
      }
      return -1;
   }
   /* Iterate through the MailBoxTable*/
   for (int i = 0; i < MAXMBOX; i++) {
   	// Find a mailbox that is unused
         if (MailBoxTable[i].status == UNUSED) {
         m_box_id = i;
        	mbox_slots_used++;
        	break;
      }
   }
  
   /* Initialize mailbox */
   MailBoxTable[m_box_id].mbox_id = m_box_id;
   MailBoxTable[m_box_id].status = USED;
   MailBoxTable[m_box_id].num_slots = slots;
   MailBoxTable[m_box_id].max_slot_size = slot_size;
   MailBoxTable[m_box_id].slots = 0;
   MailBoxTable[m_box_id].blocked_procs = 0;
   
   // Returns unique mail box ID
   return MailBoxTable[m_box_id].mbox_id;
   
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

   /* Check if message size is valid*/
   if ((msg_size < 0) || (msg_size > MAX_MESSAGE)) {
      console("The message size is not valid.\n");
      return(-1);     
   }

      /* Check if the mailbox ID is within range */
   if ((mbox_id < 1) || (mbox_id > MAXMBOX)) {
      console("The message box ID is not valid.\n");
      return(-1);     
   }

   // pointer to Mbox
   mboxPtr mbox_ptr = &MailBoxTable[mbox_id];

   /* Check if messages are availabe: if not block process until a message available*/
   if (mbox_ptr->num_slots != 0 && msg_size > mbox_ptr->max_slot_size)
   {
      enableInterrupts();
      return -1;
   }

   int pid = getpid();
   Phase2_ProcTable[pid % MAXPROC].pid = pid;
   Phase2_ProcTable[pid % MAXPROC].status = ACTIVE;
   Phase2_ProcTable[pid % MAXPROC].message = msg_ptr;
   Phase2_ProcTable[pid % MAXPROC].msg_size = msg_size;

   //block and adds to block_sendlist
   if (mbox_ptr->num_slots <= mbox_ptr->mbox_slots_used && mbox_ptr->block_recvlist == NULL)
   {
      if (mbox_ptr->block_sendlist == NULL)
      {
         mbox_ptr->block_sendlist = &Phase2_ProcTable[pid % MAXPROC];
      }
      else
      {
         mbox_proc_ptr temp = mbox_ptr->block_sendlist;
         while (temp->next_block_send != NULL)
         {
            temp = temp->next_block_send;
         }
         temp->next_block_send = &Phase2_ProcTable[pid % MAXPROC];
      }

      block_me(SEND_BLOCK);
      if(Phase2_ProcTable[pid % MAXPROC].mbox_release)
      {
         enableInterrupts();
         return -3;

      }
      return is_zapped();
   }

   /* -----------------------------------------------------
     check the process to see if on receive block list,
     if so, copy the message to the receive process buffer
   ------------------------------------------------------- */
   
   
   if (mbox_ptr->block_recvlist != NULL)
   {
      //msg_size is bigger than receieve buffer size, unblock
      if (msg_size > mbox_ptr->block_recvlist->msg_size)
      {
         mbox_ptr->block_recvlist->status = FAILED;
         int pid = mbox_ptr->block_recvlist->pid;
         mbox_ptr->block_recvlist = mbox_ptr->block_recvlist->next_ptr;
         unblock_proc(pid);
         enableInterrupts();
         return -1;
      }

      // now copy the message to receieve process buffer
      memcpy(mbox_ptr->block_recvlist->message, msg_ptr, msg_size);
      mbox_ptr->block_recvlist->msg_size = msg_size;

      int recvPid = mbox_ptr->block_recvlist->next_ptr;
      unblock_proc(recvPid);
      enableInterrupts();
      return is_zapped();
   }

   /* Need to check for mail slot table overflows*/
   int slot = get_slot_index();
   // If found halt(1)
   if (slot == -2)
   {
      console("MboxSend(): No slots in system. Halting...\n");
      halt(1);
   }
      

   /* If conditional send, mail slot table overflow does not halt(1): returns -2 */

   // initialize the slot
   slot_ptr added_slot = init_slot(slot, mbox_ptr->mbox_id, msg_ptr, msg_size);

   //places slot into slotlist
   add_slot_list(added_slot, mbox_ptr);

   enableInterrupts();
   return is_zapped();

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

   if ((mbox_id < 0) || (mbox_id > MAXMBOX)) {
      console("The message box ID is not valid.\n");
      return(-1);      
   }

      /* check if messages in mailbox*/
   if (MAX_MESSAGE < 0)
   {
      enableInterrupts();
      return -1;
   }

   //mailbox pointer
   mboxPtr mbox_ptr = &MailBoxTable[mbox_id];

   // add to the process table
   int pid = getpid();
   Phase2_ProcTable[pid % MAXPROC].pid = pid;
   Phase2_ProcTable[pid % MAXPROC].status = ACTIVE;
   Phase2_ProcTable[pid % MAXPROC].message = msg_ptr;
   Phase2_ProcTable[pid % MAXPROC].msg_size = MAX_MESSAGE;

   //slots pointer
   slot_ptr slotPtr = mbox_ptr->slots;

   if (slotPtr == NULL)
   {
      // add receive process to own blocked receive list
      if (mbox_ptr->block_recvlist == NULL)
      {
         mbox_ptr->block_recvlist = &Phase2_ProcTable[pid % MAXPROC];

      }
      else
      {
         mbox_proc_ptr temp = mbox_ptr->block_recvlist;

         while (temp->next_block_recv != NULL)
      {
         temp = temp->next_block_recv;
      }
         temp->next_block_recv = &Phase2_ProcTable[pid % MAXPROC];
      }

   

      block_me(RECV_BLOCK);

      // IF MBOX WAS RELEASED OR ZAPPED
      if(Phase2_ProcTable[pid % MAXPROC].mbox_release || is_zapped())
      {
         enableInterrupts();
         return -3;
      }
      // IF IT FAILS TO RECEIVE MESSAGE
      if (Phase2_ProcTable[pid % MAXPROC].status == FAILED)
      {
         enableInterrupts();
         return -1;
      }
      enableInterrupts();
      return Phase2_ProcTable[pid % MAXPROC].msg_size;

      // use memcpy() on message from slot to receiver's buffer if one or more messages available 
   }
   else
   {

      if (slotPtr->msg_size > MAX_MESSAGE)
      {
         enableInterrupts();
         return -1;
      }

      memcpy(msg_ptr, slotPtr->message, slotPtr->msg_size);
      mbox_ptr->slots = slotPtr->next_slot;
      int msgSize = slotPtr->msg_size;


   // Free mail slot
   zero_slot(slotPtr->slot_id);
   mbox_ptr->mbox_slots_used --;

      /* If no messages in the mailbox: block reciever */
      if (mbox_ptr->block_sendlist != NULL)
      {
         int slot_index = get_slot_index();

         slot_ptr new_slot = init_slot(slot_index, mbox_ptr->mbox_id, 
                                       mbox_ptr->block_sendlist->message,
                                       mbox_ptr->block_sendlist->msg_size);
         add_slot_list(new_slot, mbox_ptr);

         int pid = mbox_ptr->block_sendlist->pid;
         mbox_ptr->block_sendlist = mbox_ptr->block_sendlist->next_block_send;
         unblock_proc(pid);

      }

      enableInterrupts();
      return iz_zapped();
   }
} /* MboxReceive */

/* ------------------------------------------------------------------------
   Name - MboxRelease
   Purpose - Releases the MBOX, alerts any blocked processess waiting on MBOX
   Parameters - Mbox_id
   Returns - 0, -1, -3
   Side Effects - zeros Mbox and laerts blocke procs
   ----------------------------------------------------------------------- */
int MboxRelease(int mbox_id)
{
   check_kernel_mode("MboxRelease");
   disableInterrupts();

   //check to see if the mbox is valid
   if (mbox_id < 0 || mbox_id >= MAXMBOX)
   {
      enableInterrupts();
      return -1;
   }

   //check and see if Mbox was created
   if (MailBoxTable[mbox_id].status == UNUSED)
   {
      enableInterrupts();
      return -1;
   }

   mboxPtr mbox_ptr = &MailBoxTable[mbox_id];

   //if no blocked procs
   if (mbox_ptr->block_recvlist == NULL && mbox_ptr->block_sendlist == NULL)
   {
      //zero out the MBOX
      zero_mbox(mbox_id);
      enableInterrupts();

      return is_zapped();
   }else{
      mbox_ptr->status = UNUSED;

      //make all blocked procs (send & recv) release
      while (mbox_ptr->block_sendlist != NULL)
      {
         mbox_ptr->block_sendlist->mbox_release = 1;
         int pid = mbox_ptr->block_sendlist->pid;
         mbox_ptr->block_sendlist = mbox_ptr->block_sendlist->next_block_send;
         unblock_proc(pid);
         disableInterrupts();
      }
      while (mbox_ptr->block_recvlist != NULL;)
      {
         mbox_ptr->block_recvlist->mbox_release = 1;
         int pid = mbox_ptr->block_recvlist->pid;
         mbox_ptr->block_recvlist = mbox_ptr->block_recvlist->next_block_recv;
         unblock_proc(pid);
         disableInterrupts();
      }
   }

   //clear the Mbox
   zero_mbox(mbox_id);
   enableInterrupts();
   return is_zapped();

} /* MboxRelease */
/* ------------------------------------------------------------------------
   Name - MboxCondSend
   Purpose -       
   Parameters - 
   Returns - 
   Side Effects - 
   ----------------------------------------------------------------------- */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
   /* test if in kernel mode & disable interupts */
   check_kernel_mode("MboxCondSend");
   disableInterrupts(); 

   return 0;

} /* MboxCondSend */

/* ------------------------------------------------------------------------
   Name - MboxCondReceive
   Purpose -       
   Parameters - 
   Returns - 
   Side Effects - 
   ----------------------------------------------------------------------- */
int MboxCondReceive(int mbox_id, void *msg_ptr, int max_msg_size) {
   /* test if in kernel mode & disable interupts */
   check_kernel_mode("MboxCondReceive");
   disableInterrupts(); 

   return 0;
} /* MboxCondReceive */


int check_io(){

   return 0;

} /* check_io */

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FUNCTIONS ADDED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/* --------------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks to make sure functions are being called in Kernel mode
   Parameters - *func_name keeps track of where the mode checking is being invoked.
   Returns - nothing
   Side Effects -  if system is in usermode, print appropriate error and halt.
   -------------------------------------------------------------------------------- */
void check_kernel_mode(char *func_name) {
   union psr_values caller_psr;  /*Holds the caller's PSR values*/
   char buffer[200];

   if (DEBUG2 && debugflag2) {
      sprintf(buffer, "check_kernel_mode(): called for function %s\n", func_name);
      console("%s", buffer);
   }

   /*test if in kernel mode and halts if in user mode*/
   caller_psr.integer_part = psr_get();
   if(caller_psr.bits.cur_mode == 0) {
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
   Name - clock_handler2
   Purpose - 
   Parameters - 
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
void clock_handler2(int dev, void *punit) {
   int unit = (int)punit;

   /* Check if the device is the Clock Device */
   if (dev != CLOCK_DEV) {
      if (DEBUG2 && debugflag2) {
         console("clock_handler2(): clock_handler2 called on incorrect device.\n");
      }
      return(-1);
   }
   /* Check if the units are in correct range */
   if ((unit < 0) || (unit > CLOCK_UNITS)) {
      if (DEBUG2 && debugflag2) {
         console("clock_handler2(): unit is out of range.\n");
      }
      return(-1);   
   }

   /* If device is correct */
   // variable to keep track of the calls made
   static int calls = 0;
   // dummy message used for sending during clock interrupts
   int msg = 0;

   // add clock_ms to calls (each time = +20)
   calls = calls + CLOCK_MS;

   // If we have 5 calls calls = 100 this will be true, else keep calling
   if (calls % 100 == 0) {
      MboxCondSend(unit, &msg, sizeof(msg));
   }
   // call time_slice from phase1
   time_slice();

} /* clock_handler2 */

/* --------------------------------------------------------------------------------
   Name - disk_handler
   Purpose - 
   Parameters - 
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
void disk_handler(int dev, void *punit) {
   int status;
   int result;
   int unit = (int)punit;

   /* Check if the device is the Disk Device */
   if (dev != DISK_DEV) {
      if (DEBUG2 && debugflag2) {
         console("disk_handler(): disk_handler called on incorrect device.\n");
      }
      return -1;
   }
   /* Check if the units are in correct range */
   if ((unit < 0) || (unit > DISK_UNITS)) {
      if (DEBUG2 && debugflag2) {
         console("disk_handler(): unit is out of range.\n");
      }
      return -1;   
   }
   // Read device status register
   device_input(DISK_DEV, unit, &status);
   // Conditionally send the contents of status register
   result = MboxCondSend(unit, &status, sizeof(status));
   
} /* disk_handler */

/* --------------------------------------------------------------------------------
   Name - terminal_handler
   Purpose - 
   Parameters - 
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
void terminal_handler(int dev, void *punit) {
   int status;
   int result;
   int unit = (int)punit;

   /* Check if the device is the Disk Device */
   if (dev != TERM_DEV) {
      if (DEBUG2 && debugflag2) {
         console("terminal_handler(): terminal_handler called on incorrect device.\n");
      }
      return -1;
   }
   /* Check if the units are in correct range */
   if ((unit < 0) || (unit > TERM_UNITS)) {
      if (DEBUG2 && debugflag2) {
         console("disk_handler(): unit is out of range.\n");
      }
      return -1;   
   }
   // Read device status register
   device_input(TERM_DEV, unit, &status);
   // Conditionally send the contents of status register
   result = MboxCondSend(unit, &status, sizeof(status));
   
} /* terminal_handler */

/* --------------------------------------------------------------------------------
   Name - syscall_handler
   Purpose - 
   Parameters - 
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
void syscall_handler(int dev, void *unit) {
   sysargs *sys_ptr;
   sys_ptr = (sysargs *) unit;

   /* Check if interrupt is SYSCALL_INT*/
   if (dev != SYSCALL_INT) {
      console("syscall_handler(): syscall_handler called on incorrectly. Halting.\n");
      halt(1);    
   }
   /* check if call is in range*/
   if ((sys_ptr->number < 0) || (sys_ptr->number >MAXSYSCALLS)) {
      console("syscall_handler(): syscall_handler called on incorrectly. Halting.\n");
      halt(1);       
   }

   /* Call appropriate system call handler*/
   sys_vec[sys_ptr->number](sys_ptr);
   
} /* syscall_handler */

/* --------------------------------------------------------------------------------
   Name - nullsys
   Purpose - 
   Parameters - 
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
void nullsys(sysargs *args) {
   printf("nullsys: invalid syscall %d. Halting...\n", args->number);
   halt(1);
} /* nullsys */

/* --------------------------------------------------------------------------------
   Name - waitdevice
   Purpose - 
   Parameters - 
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
int waitdevice(int type, int unit, int *status) {
   int result = 0;

   /* Check if an existing type is being used */
   if ((type != DISK_DEV) && (type != CLOCK_DEV) && (type != TERM_DEV)) {
      if (DEBUG2 && debugflag2) {
         console("waitdevice(): incorrect type. Halting..\n");
      }
      halt(1);     
   }
   
   switch (type) {
   case DISK_DEV:
      result = MboxReceive(unit, status, sizeof(int));
      break;
   case CLOCK_DEV:
      result = MboxReceive(unit, status, sizeof(int));
      break;
   case TERM_DEV:
      result = MboxReceive(unit, status, sizeof(int));
      break;
   default:
      printf("waitdevice(): bad type (%d). Halting...\n", type);
      break;
   }

   if (result == -3) {
      /* we were zapped */
      return(-1);
   }
   else {
      return 0;
   }

} /* waitdevice */

/* --------------------------------------------------------------------------------
   Name - zero_mbox
   Purpose - zeros all the elements of the mailbox ID
   Parameters - mbox_id
   Returns - 
   Side Effects -  
   -------------------------------------------------------------------------------- */
void zero_mbox(int mbox_id)
{
   MailBoxTable[mbox_id].status = UNUSED;
   MailBoxTable[mbox_id].block_recvlist = NULL;
   MailBoxTable[mbox_id].block_sendlist = NULL;
   MailBoxTable[mbox_id].slots = NULL;
   MailBoxTable[mbox_id].num_slots = -1;
   MailBoxTable[mbox_id].mbox_slots_used = -1;
   MailBoxTable[mbox_id].max_slot_size = -1;
}
/* --------------------------------------------------------------------------------
   Name - zero_slot
   Purpose - 
   Parameters - slot_id
   Returns - 
   Side Effects -  
   -------------------------------------------------------------------------------- */
void zero_slot(int slot_id)
{
   MailBoxSlots[slot_id].status = UNUSED;
   MailBoxSlots[slot_id].next_slot = NULL;
   MailBoxSlots[slot_id].mbox_id = -1;
}
/* --------------------------------------------------------------------------------
   Name - zero_mbox_slot
   Purpose -  zeroes all the elements of the process ID
   Parameters - pid
   Returns - -
   Side Effects -  
   -------------------------------------------------------------------------------- */
void zero_mbox_slot(int pid)
{
   Phase2_ProcTable[pid % MAXPROC].status = UNUSED;
   Phase2_ProcTable[pid % MAXPROC].message = NULL;
   Phase2_ProcTable[pid % MAXPROC].next_block_recv = NULL;
   Phase2_ProcTable[pid % MAXPROC].next_block_send = NULL;
   Phase2_ProcTable[pid % MAXPROC].pid = -1;
   Phase2_ProcTable[pid % MAXPROC].msg_size = -1;
   Phase2_ProcTable[pid % MAXPROC].mbox_release = 0;
}
/* --------------------------------------------------------------------------------
   Name - get_slot_index
   Purpose - to return the index of the next avaiable slot from the slot array
   Parameters - none
   Returns - -2 if no slot is avaliable
   Side Effects -  
   -------------------------------------------------------------------------------- */
int get_slot_index()
{
   for (int i = 0; i < MAXSLOTS; i++)
   {
      if (MailBoxSlots[i].status == UNUSED)
      {
         return i;
      }
   }
   return -2;
}

/* --------------------------------------------------------------------------------
   Name - init_slot
   Purpose -  initializes a new slot in the slot table
   Parameters - slot_index, mBox_id, msg_ptr, msg_size
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
slot_ptr init_slot(int slot_index, int mbox_id, void *msg_ptr, int msg_size)
{
   MailBoxSlots[slot_index].mbox_id = mbox_id;
   MailBoxSlots[slot_index].status = USED;
   memcpy(MailBoxSlots[slot_index].message, msg_ptr, msg_size);
   MailBoxSlots[slot_index].msg_size = msg_size;
   return &MailBoxSlots[slot_index];
}

/* --------------------------------------------------------------------------------
   Name - add_slot_list
   Purpose - adds a Mbox slot to the slot list of a Mbox
   Parameters - added_slot, mbox_ptr
   Returns - n
   Side Effects -  
   -------------------------------------------------------------------------------- */
   int add_slot_list(slot_ptr added_slot, mboxPtr mbox_ptr)
   {
      slot_ptr head = mbox_ptr->slots;
      if (head == NULL)
      {
         mbox_ptr->slots = added_slot;

      }
      else
      {
         while (head->next_slot != NULL)
         {
            head = head->next_slot;
         }
         head->next_slot = added_slot;
      }
      return ++mbox_ptr->mbox_slots_used;
   }

