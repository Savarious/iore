#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <assert.h>

#include "ioretypes.h"
#include "util.h"
#include "ioreaio.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

void
handle_preemptive_args (int, char**);

void
display_version ();
void
display_usage (char **);
void
display_splash ();
void
display_header ();

void
display_summary (IORE_run_t *);

static IORE_run_t *
setup_experiment (int, char**);
static void
cleanup_experiment (IORE_run_t *);
static void
setup_run (IORE_params_t *);

static void
exec_run (IORE_run_t *);

static void
setup_mpi_comm (IORE_params_t *);
static void
setup_timers (double **, int);

/*****************************************************************************
 * D E C L A R A T I O N S                                                   *
 *****************************************************************************/

static int nprocs; /* number of MPI tasks */
static int rank; /* this task MPI rank */
static enum VERBOSITY verbose; /* verbosity level */
static double *timer[NUM_TIMERS]; /* timers for each experiment run */

/* TODO: automatically check compiled interfaces */
static IORE_aio_t *available_aio[] =
  { &ioreaio_posix, NULL }; /* available abstract I/O implementations */
static IORE_aio_t *backend; /* abstract I/O implementation */

/*****************************************************************************
 * M A I N                                                                   *
 *****************************************************************************/

int
main (int argc, char **argv)
{
  IORE_run_t *experiment;
  IORE_run_t *run;

  handle_preemptive_args (argc, argv);

  display_splash ();

  /* sanity check */
  assert(available_aio[0] != NULL);

  /* initialize MPI main communicator */
  IORE_MPI_CHECK(MPI_Init (&argc, &argv),
		 "Failed to initialize MPI communicator.");
  IORE_MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nprocs),
		 "Failed to get the number of MPI tasks.");
  IORE_MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank),
		 "Failed to get the MPI rank.");

  experiment = setup_experiment (argc, argv);

  if (rank == MASTER_RANK)
    display_header ();

  /* loop through experiment runs */
  for (run = experiment; run != NULL; run = run->next)
    exec_run (run);

  if (rank == MASTER_RANK)
    {
      display_summary (experiment);

      display_footer ();
    }

  cleanup_experiment (experiment);
  IORE_MPI_CHECK(MPI_Finalize (), "Failed to finalize MPI communicator.");

  exit (EXIT_SUCCESS);
}

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Handle --help and --version command line arguments, displaying the program
 * usage and version, respectively, and exiting.
 */
void
handle_preemptive_args (int argc, char **argv)
{
  int v, h;
  int i;
  for (i = 1; i < argc; i++)
    {
      if (STREQUAL(argv[i], "--version"))
	v = 1;
      else if (STREQUAL(argv[i], "--help"))
	h = 1;
    }

  if (v)
    {
      display_version ();
    }

  if (h)
    {
      display_usage ();
    }

  if (v || h)
    {
      exit (EXIT_SUCCESS);
    }
} /* handle_preemptive_args (int, char**) */

/*
 * Execute a single run of the experiment.
 */
static void
exec_run (IORE_run_t *run)
{
  IORE_params_t *params = &run->params;

  /* synchronize all tasks */
  IORE_MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD), "Failed to synchronize tasks.");

  setup_run (params);

  /* liberate tasks not participating in this run */
  if (params->comm == MPI_COMM_NULL)
    return;
} /* exec_run (IORE_run_t *) */

static void
setup_run (IORE_params_t *params)
{
  verbose = params->verbose;

  setup_mpi_comm (params);
  /* liberate tasks not participating in this run */
  if (params->comm == MPI_COMM_NULL)
    return;
  if (rank == MASTER_RANK && verbose >= CONTROL)
    INFOF("Participating tasks: %d\n", params->adjusted_task_count);

  setup_timers (params->num_repetitions);

  /* TODO: define the workload of this task */
} /* setup_run (IORE_params_t *) */

/*
 * Setup a MPI communicator (different from MPI_COMM_WORLD) for a specific
 * experiment run.
 */
static void
setup_mpi_comm (IORE_params_t *params)
{
  MPI_Comm comm;
  MPI_Group group, newgroup;
  int range[3];
  enum VERBOSITY verbose = params->verbose;
  int task_count = params->task_count;

  /* check if there are enough tasks in the MPI communicator;
   * otherwise execute with the available number of tasks */
  if (task_count > nprocs)
    {
      if (rank == MASTER_RANK)
	{
	  WARNF("More tasks requested (%d) than available (%d); "
		"running on %d tasks.\n",
		task_count, nprocs, task_count);
	}
      params->adjusted_task_count = nprocs;
    }

  /* create new MPI communicator */
  IORE_MPI_CHECK(MPI_Comm_group(MPI_COMM_WORLD, &group),
		 "Failed to get MPI group.");
  range[0] = 0; /* first rank */
  range[1] = params->adjusted_task_count - 1; /* last rank */
  range[2] = 1; /* stride */
  IORE_MPI_CHECK(MPI_Group_range_incl (group, 1, &range, &newgroup),
		 "Failed to define new MPI group.");
  IORE_MPI_CHECK(MPI_Comm_create(MPI_COMM_WORLD, newgroup, &comm),
		 "Failed to create the new MPI communicator.");

  params->comm = comm;
} /* setup_mpi_comm (IORE_params_t *) */

static void
setup_timers (int num_repetitions)
{
  int i;
  for (i = 0; i < NUM_TIMERS; i++)
    {
      timer[i] = (double *) malloc (num_repetitions * sizeof(double));
      if (timer[i] == NULL)
	FATAL("Failed to allocate memory for timers.");
    }
} /* setup_timers (int) */
