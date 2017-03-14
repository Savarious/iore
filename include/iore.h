#ifndef _IORE_H
#define _IORE_H

#include <mpi.h>

#include "util.h"

/*****************************************************************************
 * T Y P E S   A N D   S T R U C T S                                         *
 *****************************************************************************/

typedef long long int IORE_offset_t;
typedef long long int IORE_size_t;

typedef struct {
  char api[MAX_STR]; /* API for I/O */
  int num_tasks; /* number of tasks for test */
  enum SHARING_POLICY sharing_policy; /* sharing policy for file access (shared
					 file or file per process) */

  int write; /* perform write test */
  int write_check; /* check data written after write test */
  int read; /* perform read test */
  int read_check; /* check data read after read test */
  
  int use_o_direct; /* use O_DIRECT flag (bypass I/O buffers) */
  int use_existing_test_file; /* do not remove test file before the test */

  int intra_test_delay; /* delay in seconds among tests and repetitions */
  int max_time_per_test; /* max time in seconds to run each test */
  int repetitions; /* number of repetitions of the test */
  
  IORE_offset_t offset; /* file offset for read/write */
  MPI_Comm comm; /* MPI communicator for the test */

  enum VERBOSE verbose; /* verbosity level [0, 5] */

  /* POSIX exclusive parameters */
  int fsync; /* fsync() after writing the whole file */
  int fsync_per_write; /* fsync() after each write() */
  int single_io_attempt; /* do not retry an I/O operation if incomplete */
} IORE_param_t;

typedef struct {

} IORE_results_t;

/*
 * List of tests, with its parameters and results.
 */
typedef struct IORE_test_t {
  IORE_param_t params;
  IORE_results_t *results;
  struct IORE_test_t *next;
} IORE_test_t;

#endif /* _IORE_H */
