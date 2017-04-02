#ifndef _IORE_PARAMS_H
#define _IORE_PARAMS_H

#include <sys/param.h> /* MAXPATHLEN */

#include "util.h"

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* parameters passed by the user to define an experiment */
typedef struct iore_params
{
  int num_tasks; /* number of parallel MPI tasks */
  char api[MAX_STR_LEN]; /* name of the API used for I/O */
  sharing_policy_t sharing_policy; /* policy for file sharing among tasks */
  access_pattern_t access_pattern; /* ordering patterns for file accesses */
  iore_size_t *block_sizes; /* total amount of data accessed by tasks */
  iore_size_t *transfer_sizes; /* amount of data accessed in a single request */
  
  int write_test; /* execute the write performance test */
  int read_test; /* execute the read performance test */

  int ref_num; /* custom experiment reference number */
  char root_file_name[MAXPATHLEN]; /* full name provided for the test file */
  int num_repetitions; /* number of repetitions for each run */
  int inter_test_delay; /* delay in seconds before read/write tests */
  int intra_test_barrier; /* sync tasks before and after read/write tests */
  int run_time_limit; /* time soft limit in minutes to complete each run */
  int keep_file; /* keep the test file after test completion */
  int use_existing_file; /* execute read/write tests on existing test files */
  int use_rep_in_file_name; /* use the repetition number in the file name */
  int dir_per_file; /* create an individual directory for each test file */
  int reorder_tasks; /* in read tests, a task reads offsets of other task */
  int reorder_tasks_offset; /* distances in number of ranks for reordering */

  /* POSIX specific parameters */
  int single_io_attempt; /* do not retry a transfer if incomplete */

  /* control parameters not provided by the user */
  int block_sizes_length;
  int transfer_sizes_length;
} iore_params_t;

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

/*
 * Create a default parameter structure.
 */
iore_params_t *new_params ();

#endif /* _IORE_PARAMS_H */
