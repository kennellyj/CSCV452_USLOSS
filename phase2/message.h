#define DEBUG2 1

typedef struct mailbox mail_box;

typedef struct mail_slot mail_slot;
typedef struct mail_slot *slot_ptr;

typedef struct mbox_proc mbox_proc;
typedef struct mbox_proc *mbox_proc_ptr;

struct mailbox {
   int           mbox_id;
   int           status;
   int           num_slots;
   int           max_slot_size;
   slot_ptr      slots;
   mbox_proc_ptr blocked_procs; 
};

struct mail_slot {
   int       mbox_id;
   int       status;
   char      message[MAX_MESSAGE];
   slot_ptr  next_slot;
};

struct mbox_proc {
   int           pid;
   mbox_proc_ptr next_ptr;
};

struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
   struct psr_bits bits;
   unsigned int integer_part;
};

#define USED 1
#define UNUSED 0

#define MAILBOX_TABLE 1
#define SLOT_TABLE    2
#define PROC_TABLE    3
