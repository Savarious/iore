#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "iore_experiment.h"
#include "iore_run.h"
#include "iore_params.h"
#include "iore_aio.h"
#include "iore_task.h"
#include "display.h"
#include "util.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       
 *****************************************************************************/

/* options handling */
static int handle_preemptive_args (int, char **);
/* MPI related functions */
static void init_mpi (int, char**);
static void finalize_mpi ();

/*****************************************************************************
 * G L O B A L S
 *****************************************************************************/

/* task context variables */
iore_task_t *task;

/*****************************************************************************
 * F I L E  G L O B A L S
 *****************************************************************************/

/* I/O APIs available */
static iore_aio_t *available_aio[] = {
#ifdef USE_POSIX_AIO
  &ioreaio_posix,
#endif
  NULL
};

/*****************************************************************************
 * M A I N                                                                   
 *****************************************************************************/

int
main (int argc, char **argv)
{
  iore_experiment_t *experiment;
  
  init_mpi(argc, argv);
  
  if (!handle_preemptive_args(argc, argv))
    {
      display_splash();

      /* sanity check */
      if (available_aio[0] == NULL)
	{
	  FATAL("No I/O API compiled into IORE; aborting.");
	  MPI_Abort(MPI_COMM_WORLD, -1);
	}

      /* prepare experiments */
      experiment = new_experiment(argc, argv);
      task->verbosity = experiment->verbosity;
      /* TODO: validate experiments? */
      display_expt_header(argv);
      
    }

  finalize_mpi();

  exit (EXIT_SUCCESS);
} /* main (int, char **) */

/*****************************************************************************
 * F U N C T I O N S                                                         
 *****************************************************************************/

/*
 * Handle --help and --version command line arguments, displaying program usage
 * and version. It returns TRUE if any of these arguments are found, FALSE
 * otherwise.
 */
static int
handle_preemptive_args (int argc, char **argv)
{
  int i = 1;
  int v = FALSE;
  int h = FALSE;

  while (i < argc && (!v || !h))
    {
      if (STREQUAL(argv[i], "--version"))
	v = TRUE;
      else if (STREQUAL(argv[i], "--help"))
	h = TRUE;
      
      i++;
    }

  if (v)
    display_version();

  if (h)
    display_usage(argv);

  return (v || h);
} /* handle_preemptive_args (int, char **) */

/*
 * Start MPI main communicator.
 */
static void
init_mpi (int argc, char **argv)
{
  int nprocs, rank;

  MPI_TRYCATCH(MPI_Init(&argc, &argv),
	       "Failed to initialize MPI communicator.");
  MPI_TRYCATCH(MPI_Comm_size(MPI_COMM_WORLD, &nprocs),
	       "Failed to get the number of MPI tasks.");
  MPI_TRYCATCH(MPI_Comm_rank(MPI_COMM_WORLD, &rank),
	       "Failed to get the MPI rank.");

  /* initialize task context */
  task = new_task(nprocs, rank, MPI_COMM_WORLD);
} /* init_mpi (int, char **) */

static void
finalize_mpi ()
{
  free(task);
  
  MPI_TRYCATCH(MPI_Finalize(), "Failed to gracefully finalizing MPI.");
} /* finalize_mpi() */
