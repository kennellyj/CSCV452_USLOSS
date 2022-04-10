
#define DEBUG3 1

#define ACTIVE 1

typedef struct mbox_proc mbox_proc;
typedef struct mbox_proc *mbox_proc_ptr;

typedef struct sem_proc semaphore;
typedef struct sem_proc *sem_proc_ptr;

struct mbox_proc {
   short         pid;
   short         parent_pid;
   int           status;
   char         *name;
   int           numChild;
   int           mbox_start;
   char         *start_arg;
   int          (* start_func)(char *);
   unsigned int stack_size;
   int          priority;
   int          spawnbox;
   mbox_proc_ptr parent_ptr;
   mbox_proc_ptr child_ptr;
   mbox_proc_ptr sibling_ptr;
};

struct sem_proc {
    int         mutex_box;
    int         blocked_box;
    int         value;
    int         blocked;
};
