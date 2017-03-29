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
*new_task (int nprocs, int rank)
{
  iore_task_t *task;

  task = (iore_task_t *) malloc(sizeof(iore_task_t));
  task->nprocs = nprocs;
  task->rank = rank;

  set_wclock_deviation(task);

  return (task);
} /* new_task (int, int) */

static void
set_wclock_deviation (iore_task_t *task)
{
  iore_time_t time;
  iore_time_t min;
  iore_time_t max;
  iore_time_t master_time;
  
  MPI_TRYCATCH(MPI_Barrier(MPI_COMM_WORLD), "Failed tasks synchronization.");
  time = current_time();

  MPI_TRYCATCH(MPI_Reduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, MASTER_RANK,
			  MPI_COMM_WORLD),
	       "Failed to reduce tasks' timestamps.");
  MPI_TRYCATCH(MPI_Reduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, MASTER_RANK,
			  MPI_COMM_WORLD),
	       "Failed to reduce tasks' timestamps.");

  /* compute wall-clock time delta comparing with master rank time */
  master_time = time;
  MPI_TRYCATCH(MPI_Bcast(&master_time, 1, MPI_DOUBLE, MASTER_RANK,
			 MPI_COMM_WORLD),
	       "Failed to broadcast master rank time.");
  task->wclock_delta = master_time - time;

  /* wall-clock skew considers min and max times across all tasks */
  task->wclock_skew_all = max - min;
} /* set_wclock_deviation (iore_task_t) */
