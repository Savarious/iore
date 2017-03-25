#include <mpi.h>

#include "iore_run.h"

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

/*
 * Create an experiment run with ID and params.
 */
iore_run_t *
new_run (int id, iore_params_t *params)
{
  iore_run_t *run = NULL;

  if (params == NULL)
    {
      FATAL("Parameters are mandatory to define an experiment run.");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  run = (iore_run_t *) malloc(sizeof(iore_run_t));
  if (run == NULL)
    {
      FATAL("Failed to allocate memory for run structure");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  run->id = id;
  run->params = *params;
  run->next = NULL;

  return (run);
} /* new_run (int, iore_params_t *) */
