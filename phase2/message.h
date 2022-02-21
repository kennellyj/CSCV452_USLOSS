#define DEBUG2 1

typedef struct mail_slot *slot_ptr;
typedef struct mailbox mail_box;
typedef struct mbox_proc *mbox_proc_ptr;

struct mailbox {
   int           mbox_id;
   /* other items as needed... */
// int           num_slots
// int           max_slot_size
// int           slots_used
// mbox_proc_ptr my_slots
// mbox_proc_ptr blocked_procs
};

struct mail_slot {
   int       mbox_id;
   int       status;
   /* other items as needed... */
// char      message[m]
// mail_slot slot_ptr* next_slot

};

/*
struct mbox_proc_ptr {
   int      pid;
   mbox_proc_ptr* next_ptr
}
*/

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
