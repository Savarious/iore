#include <mpi.h>
#include <string.h>

#include "iore_params.h"
#include "util.h"

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

/*
 * Create a default parameter structure.
 */
iore_params_t *
new_params ()
{
  iore_params_t *params = NULL;
  iore_size_t block_sizes[1] = { 4096 };
  iore_size_t transfer_size[1] = { 4096 };

  params = (iore_params_t *) malloc(sizeof(iore_params_t));
  if (params == NULL)
    {
      FATAL("Failed to allocate memory for parameters structure");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  params->num_tasks = 1;
  strcpy(params->api, "POSIX");
  params->sharing_policy = SHARED_FILE;
  params->access_pattern = SEQUENTIAL;
  params->block_sizes = block_sizes;
  params->transfer_sizes = transfer_size;

  params->write_test = TRUE;
  params->read_test = TRUE;

  params->ref_num = -1;
  strcpy(params->root_file_name, "testfile");
  params->num_repetitions = 1;
  params->inter_test_delay = 0;
  params->intra_test_barrier = FALSE;
  params->run_time_limit = 0;
  params->keep_file = FALSE;
  params->use_existing_file = FALSE;
  params->use_rep_in_file_name = FALSE;
  params->dir_per_file = FALSE;
  params->reorder_tasks = FALSE;
  params->reorder_tasks_offset = 0;

  params->single_io_attempt = FALSE;

  return (params);
} /* new_params () */
