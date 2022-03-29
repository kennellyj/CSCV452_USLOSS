/*
 *
 * sems.h
 * 
 *     Justin Kennelly & Carlos Torres
 * CSCV 452 Phase 3
 *
 */

 
 #ifndef PHASE3_HEADER_H_
 #define PHASE3_HEADER_H_
 
 //structures defs below


// procTable status #'s
#define UNUSED      0
#define ACTIVE      1


#endif /* PHASE3_HEADER_H_ */

typedef struct Phase3ProcTable *ProcTablePtr;

typedef struct Phase3ProcTable{
    //add more functions here
    char    name[MAXNAME];
    short   pid;
    int     priority;
    unsigned int    stack_size;
    int     status;
    int     mboxID;
}Phase3ProcTable;