#include <mpi.h>

#include "iore_task.h"
#include "util.h"

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

static void set_wclock_deviation (iore_task_t *);

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

/*
 * Create a new task context.
 */
iore_task_t
*new_task (int nprocs, int rank, MPI_Comm comm)
{
  iore_task_t *task;

  task = (iore_task_t *) malloc(sizeof(iore_task_t));
  task->nprocs = nprocs;
  task->rank = rank;
  task->comm = comm;
  task->verbosity = NORMAL;
  task->wclock_delta = 0;
  task->wclock_skew_all = 0;

  set_wclock_deviation(task);

  return (task);
} /* new_task (int, int, MPI_Comm) */

static void
set_wclock_deviation (iore_task_t *task)
{
  iore_time_t time;
  iore_time_t min = 0;
  iore_time_t max = 0;
  iore_time_t master_time;
  
  MPI_TRYCATCH(MPI_Barrier(MPI_COMM_WORLD), "Failed syncing tasks.");
  time = MPI_Wtime();
  if (time < 0)
    FATAL("Failed to get timestamp using MPI_Wtime().");

  MPI_TRYCATCH(MPI_Reduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, MASTER_RANK,
			  MPI_COMM_WORLD),
	       "Failed to reduce tasks' timestamps.");
  MPI_TRYCATCH(MPI_Reduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, MASTER_RANK,
			  MPI_COMM_WORLD),
	       "Failed to reduce tasks' timestamps.");

  /* compute wall-clock time delta comparing with master rank time */
  if (task->rank == MASTER_RANK)
    master_time = time;
  MPI_TRYCATCH(MPI_Bcast(&master_time, 1, MPI_DOUBLE, MASTER_RANK,
			 MPI_COMM_WORLD),
	       "Failed to broadcast master rank time.");
  task->wclock_delta = master_time - time;

  /* wall-clock skew considers min and max times across all tasks */
  task->wclock_skew_all = max - min;
} /* set_wclock_deviation (iore_task_t) */
