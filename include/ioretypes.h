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

typedef struct IORE_params
{
  int ref_num; /* custom ID number */

  int num_tasks; /* number of parallel tasks */
  char api[MAX_STR]; /* API used for I/O */
  enum SHARING_POLICY sharing_policy; /* policy for file sharing */
  enum ACCESS_PATTERN access_pattern; /* access order of file offsets */
  IORE_size_t *block_sizes; /* set of sizes of blocks */
  IORE_size_t *transfer_sizes; /* set of sizes of I/O requests */
  int write_test; /* perform write test */
  int read_test; /* perform read test */
  char file_name[MAXPATHLEN]; /* full name of the provided test file */

  int num_repetitions; /* number of repetitions of a run */
  int inter_test_delay; /* delay in seconds before read/write tests */
  int intra_test_barrier; /* synchronize tasks before and after I/O tests */
  int run_time_limit; /* time soft limit in minutes to complete each run */
  int keep_file; /* keep the file after test completion */
  int use_existing_file; /* prevent removing the file before the test */
  int rep_in_file_name; /* use repetition number to form the file name */
  int dir_per_file; /* use an individual directory for each file */
  int reorder_tasks; /* tasks read offsets from other tasks */
  int reorder_tasks_offset; /* distance in number of nodes for reordering */
  enum VERBOSITY verbose; /* verbosity level */

  MPI_Comm comm; /* MPI communicator for the run */
  int rank; /* MPI rank */
  int eff_num_tasks; /* effective number of tasks for a run */
  int tasks_per_node; /* number of tasks per compute node */
  int repetition; /* repetition number inside a run */
  IORE_size_t transfer_size; /* transfer size used by a task */
  IORE_offset_t offset; /* current offset being read/written */
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
  (*create) (IORE_params_t *);
  void *
  (*open) (IORE_params_t *);
  void
  (*close) (void *, IORE_params_t *);
  void
  (*delete) (IORE_params_t *);
  IORE_size_t
  (*io) (void *, IORE_size_t *, IORE_size_t, enum ACCESS_TYPE, IORE_params_t *);
} IORE_aio_t;

#endif /* not _IORETYPES_H */
