#ifndef _UTIL_H
#define _UTIL_H

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define MASTER_RANK 0 /* MPI master rank */
#define MAX_RETRIES 10000 /* max number of retries for POSIX I/O */
#define MAX_STR_LEN 1024 /* max length of a string inside the program */

/* enumeration of supported verbosity levels */
typedef enum verbosity
  {
    QUIET, NORMAL, VERBOSE, VERY_VERBOSE, DEBUG
  } verbosity_t;

/* enumeration of file sharing policies */
typedef enum sharing_policy
  {
    SHARED_FILE, FILE_PER_PROCESS
  } sharing_policy_t;

/* enumeration of access pattern in terms of file offsets */
typedef enum access_pattern
  {
    SEQUENTIAL, RANDOM
  } access_pattern_t;

/* enumeration of supported access type tests */
typedef enum access
  {
    READ, WRITE
  } access_t;

typedef double iore_time_t; /* execution time */
typedef long long int iore_offset_t; /* file offset */
typedef long long int iore_size_t; /* sizes in byte units or multiples */

/* enumeration of collected timers; W for write, R for read, and D for delete */
enum timer
  {
    W_OPEN_START,
    W_OPEN_STOP,
    W_START,
    W_STOP,
    W_CLOSE_START,
    W_CLOSE_STOP,
    R_OPEN_START,
    R_OPEN_STOP,
    R_START,
    R_STOP,
    R_CLOSE_START,
    R_CLOSE_STOP,
    D_START,
    D_STOP,
    NUM_TIMERS
  };

#endif /* _UTIL_H */
