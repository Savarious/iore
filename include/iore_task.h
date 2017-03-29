#ifndef _IORE_TASK_H
#define _IORE_TASK_H

#include <mpi.h>
#include <util.h>

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* execution context of a task */
typedef struct iore_task
{
  int rank; /* MPI rank */
  int nprocs; /* number of processes in MPI_COMM_WORLD */
  MPI_Comm comm; /* MPI communicator for experiment runs */
  verbosity_t verbosity; /* verbosity level */
  iore_time_t wclock_delta; /* time difference regarding master rank */
  iore_time_t wclock_skew_all; /* time difference across all tasks */
} iore_task_t;

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

/*
 * Create a new task context.
 */
iore_task_t *new_task(int, int);

#endif /* _IORE_TASK_H */
