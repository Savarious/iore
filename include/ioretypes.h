#ifndef _IORETYPES_H
#define _IORETYPES_H

#include <mpi.h>

#include "util.h"

/*****************************************************************************
 * E N U M S ,   T Y P E S   A N D   S T R U C T S                           *
 *****************************************************************************/

enum SHARING_POLICY
{
  SHARED_FILE, FILE_PER_PROCESS
};

enum ACCESS_PATTERN
{
  SEQUENTIAL, RANDOM
};

enum ACCESS_TYPE
{
  READ, WRITE
};

enum VERBOSITY
{
  BASIC, CONTROL, ENVIRONMENT, TASK
};

typedef long long int IORE_offset_t;
typedef long long int IORE_size_t;
typedef double IORE_time_t;

typedef struct IORE_workload
{
  int num_tasks; /* number of parallel tasks */
  char api[MAX_STR]; /* API used for I/O */
  enum SHARING_POLICY sharing_policy; /* policy for file sharing */
  enum ACCESS_PATTERN access_pattern; /* access order of file offsets */
  IORE_size_t *block_size; /* set of sizes of blocks */
  IORE_size_t *transfer_size; /* set of sizes of I/O requests */
  int write_test; /* perform write test */
  int read_test; /* perform read test */
  char *orig_file_name; /* full name of the test file defined by the user*/
} IORE_workload_t;

typedef struct IORE_params
{
  int ref_num; /* custom ID number */
  IORE_workload_t *workloads; /* set of workloads */
  int num_repetitions; /* number of repetitions of a run */
  int inter_test_delay; /* delay in seconds before read/write tests */
  int intra_test_barrier; /* synchronize tasks before and after I/O tests */
  int test_time_limit; /* time soft limit in minutes to complete each test */
  int keep_file; /* keep the file after test completion */
  int keep_file_on_error; /* keep the file after a test with error */
  int use_existing_file; /* prevent removing the file before the test */
  enum VERBOSITY verbose; /* verbosity level */

  MPI_Comm comm; /* MPI communicator for the run */
  int task_count; /* total number of tasks specified for a run */
  int adjusted_task_count; /* adjusted number of tasks for a run */
} IORE_params_t;

typedef struct IORE_results
{
  IORE_time_t **write_time; /* write time of each task */
  IORE_time_t **read_time; /* read time of each task */
  IORE_time_t *write_total_time; /* time for all tasks to complete write test */
  IORE_time_t *read_total_time; /* time for all tasks to complete read test */
} IORE_results_t;

typedef struct IORE_run
{
  int id; /* unique ID */
  IORE_params_t params; /* parameters */
  IORE_results_t results; /* results of all repetitions */
  struct IORE_run *next; /* next run */
} IORE_run_t;

typedef struct IORE_aio
{
  char *name; /* API name */

  void *
  (*create) (char *, IORE_params_t *);
  void *
  (*open) (char *, IORE_params_t *);
  void
  (*close) (void *, IORE_params_t *);void
  (*delete) (char *, IORE_params_t *);
  IORE_size_t
  (*io) (void *, IORE_size_t *, IORE_size_t, enum ACCESS_TYPE, IORE_params_t *);
} IORE_aio_t;

#endif /* not _IORETYPES_H */
