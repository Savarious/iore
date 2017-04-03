#ifndef _IORE_TASK_H
#define _IORE_TASK_H

#include <mpi.h>
#include <util.h>
#include <iore_aio.h>

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* execution context of a task */
typedef struct iore_task
{
  int run_id; /* ID of the current run */
  MPI_Comm comm; /* MPI communicator for experiment runs */
  int nprocs; /* number of processes in MPI_COMM_WORLD */
  int rank; /* MPI rank */
  verbosity_t verbosity; /* verbosity level */
  iore_time_t wclock_delta; /* time difference regarding master rank */
  iore_time_t wclock_skew_all; /* time difference across all tasks */
  iore_time_t *timer[NUM_TIMERS]; /* performance timers for all repetitions */
  iore_size_t *data_moved[2]; /* amount of data moved in read/write tests */
  iore_aio_t *aio_backend; /* abstract I/O implementation */
  unsigned long long data_signature; /* data signature pattern */
  char *test_file_name; /* full path of the test file */
  iore_size_t block_size; /* size of a sequential block of data accessed */
  iore_size_t transfer_size; /* size of I/O requests */
} iore_task_t;

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

/*
 * Create a new task context.
 */
iore_task_t *new_task(int, int, MPI_Comm);

#endif /* _IORE_TASK_H */
