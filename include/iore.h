#ifndef _IORE_H
#define _IORE_H

#include <mpi.h>

#include "util.h"

/*****************************************************************************
 * D E F I N I T I O N S                                                     *
 *****************************************************************************/


/* Timer array indices; W for write, R for read, and D for delete */
#define NUM_TIMERS 14 /* number of timers */
#define W_OPEN_START 0
#define W_OPEN_STOP 1
#define W_START 2
#define W_STOP 3
#define W_CLOSE_START 4
#define W_CLOSE_STOP 5
#define R_OPEN_START 6
#define R_OPEN_STOP 7
#define R_START 8
#define R_STOP 9
#define R_CLOSE_START 10
#define R_CLOSE_STOP 11
#define D_START 12
#define D_STOP 13

/*****************************************************************************
 * T Y P E S   A N D   S T R U C T S                                         *
 *****************************************************************************/

typedef long long int IORE_offset_t;
typedef long long int IORE_size_t;

/*
 * Test parameters.
 */
typedef struct {
	int id; /* test unique ID */

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
	int keep_file; /* keep file after test completion */
	int keep_file_with_error; /* keep file after test in case of error */

	int intra_test_delay; /* delay in seconds among tests and repetitions */
	int intra_test_sync; /* synchronize tasks between open and write/read and
	 between write/read and close */
	int max_time_per_test; /* max time in seconds to run each test */
	int repetitions; /* number of repetitions of the test */

	IORE_offset_t offset; /* file offset for read/write */
	MPI_Comm comm; /* MPI communicator for the test */
	int error_found; /* error found at write/read check */

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
