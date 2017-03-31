#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
static void setup_mpi_comm (iore_params_t *);
/* experiment execution related functions */
static void exec_run (iore_run_t *);
static void exec_repetition (int, iore_time_t, iore_params_t *);
static void exec_write_test (int, iore_params_t *);
static void exec_read_test (int, iore_params_t *);
static void setup_run (iore_params_t *);
static void setup_timers (int);
static void bind_aio_backend (char *);
static void setup_data_signature ();
static char *get_file_name (iore_params_t *);

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
  &iore_aio_posix,
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
  iore_run_t *run;
  
  init_mpi(argc, argv);
  
  if (!handle_preemptive_args(argc, argv))
    {
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
      sync_rand_gen(MPI_COMM_WORLD);

      display_splash();
      
      display_expt_header(argc, argv);

      /* loop over experiment runs */
      for (run = experiment->front; run != NULL; run = run->next)
	exec_run(run);

      /* TODO: continue with results output... */
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

/*
 * Gracefully finalize MPI environment and other variables.
 */
static void
finalize_mpi ()
{
  free(task);
  
  MPI_TRYCATCH(MPI_Finalize(), "Failed to gracefully finalizing MPI.");
} /* finalize_mpi() */

/*
 * Setup the MPI communicator used for a specific experiment run.
 */
static void
setup_mpi_comm (iore_params_t *params)
{
  MPI_Comm newcomm;
  MPI_Group group, newgroup;
  int range[3];

  /* if the number of tasks requested is greater than the available in the
     communicator, use the available */
  if (params->num_tasks > task->nprocs)
    {
      if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
	{
	  WARNF("More tasks requested (%d) than available (%d); "
		"running on %d tasks", params->num_tasks, task->nprocs,
		task->nprocs);
	}
      params->num_tasks = task->nprocs;
    }

  /* create an MPI communicator for this run */
  MPI_TRYCATCH(MPI_Comm_group(MPI_COMM_WORLD, &group),
	       "Failed to get current MPI group.");
  range[0] = 0; /* first rank */
  range[1] = params->num_tasks - 1; /* last rank */
  range[2] = 1; /* stride */
  MPI_TRYCATCH(MPI_Group_range_incl(group, 1, &range, &newgroup),
	       "Failed to define new MPI group.");
  MPI_TRYCATCH(MPI_Comm_create(MPI_COMM_WORLD, newgroup, &newcomm),
	       "Failed to create new MPI communicator.");

  task->comm = newcomm;
} /* setup_mpi_comm (iore_params_t *) */

/*
 * Execute all iterations of an experiment run.
 */
static void
exec_run (iore_run_t *run)
{
  iore_params_t *params = &run->params;
  iore_time_t deadline;
  int i;

  setup_run(params);
  deadline = current_time() + params->run_time_limit;
  display_run_info(run->id, params);
  
  /* only tasks participating in this run */
  if (task->comm != MPI_COMM_NULL)
    {
      /* loop over run replications */
      for (i = 0; i < params->num_repetitions; i++)
	exec_repetition(i, deadline, params);
      
      /* TODO: continue... */
    }

  /* TODO: cleanup run */
  
} /* exec_run (iore_run_t *) */

/*
 * Execute a single iteration of an experiment run.
 */
static void
exec_repetition (int r, iore_time_t deadline, iore_params_t *params)
{
  /* uses a different data signature for each iteration */
  setup_data_signature();
  
  if (r == 0)
    display_rep_header();

  /* write performance test */
  if (params->write_test &&
      (params->run_time_limit == 0 || current_time() < deadline))
    exec_write_test(r, params);

  /* read performance test */
  if (params->read_test &&
      (params->run_time_limit == 0 || current_time() < deadline))
    exec_read_test(r, params);

  /* finalizing iteration */
  if (!params->keep_file)
    {
      task->timer[D_START][r] = current_time();
      /* TODO: remove file */
      task->timer[D_STOP][r] = current_time();

      MPI_TRYCATCH(MPI_Barrier(task->comm), "Failed syncing tasks.");

      /* TODO: display remove results */
    }
} /* exec_repetition (int, iore_time_t deadline, iore_params_t *); */

/*
 * Execute a write performance test.
 */
static void
exec_write_test (int r, iore_params_t *params)
{
  char *file_name;

  file_name = get_file_name(params);
} /* exec_write_test (int, iore_params_t *) */

/*
 * Execute a read performance test.
 */
static void
exec_read_test (int r, iore_params_t *params)
{
  char *file_name;

  /* TODO: setup rank offset */
  
  file_name = get_file_name(params);
} /* exec_read_test (int, iore_params_t *) */

/*
 * Prepare the execution of an experiment run.
 */
static void
setup_run (iore_params_t *params)
{
  setup_mpi_comm (params);

  /* only tasks participating in this run */
  if (task->comm != MPI_COMM_NULL)
    {
      setup_timers(params->num_repetitions);
      bind_aio_backend(params->api);
    }
} /* setup_run (iore_params_t *) */

/*
 * Reset performance timers to collect results from an experiment run.
 */ 
static void
setup_timers (int num_repetitions)
{
  int i;
  for (i = 0; i < NUM_TIMERS; i++)
    {
      task->timer[i] = (double *) malloc(num_repetitions * sizeof(iore_time_t));
      if (task->timer[i] == NULL)
	FATAL("Failed to setup performance timers.");
    }
} /* setup_timers (int) */

/*
 * Bind an abstract I/O implementation to the backend.
 */
static void
bind_aio_backend (char *api)
{
  iore_aio_t *backend = NULL;
  iore_aio_t **aio = available_aio;

  while (*aio != NULL && backend == NULL)
    {
      if (STREQUAL(api, (*aio)->name))
	backend = *aio;

      aio++;
    }

  if (backend == NULL)
    FATALF("Unrecognized abstract I/O API: %s.", api);
  else
    task->aio_backend = backend;
} /* bind_aio_backend (char *) */

/*
 * Generates a data signatures to be used in write tests.
 */
static void
setup_data_signature ()
{
  time_t curtime;

  curtime = time(NULL);
  if (curtime == -1)
    FATAL("Failed to generate data signature.");

  task->data_signature = curtime;
} /* setup_data_signature () */

/*
 * Returns the actual file name used for tests. It may include suffixes and
 * prefixes depending on supplied parameters.
 */
static char *
get_file_name (iore_params_t *params)
{
  char *file_name = (char *) malloc(MAX_STR_LEN * sizeof(char));
  
  if (params->sharing_policy == SHARED_FILE)
    strcpy (file_name, params->root_file_name);
  else
    {
      if (params->dir_per_file)
	{
	  /* TODO: continue... */
	}
    }

  /* TODO: continue... */

  return (file_name);
} /* get_file_name (iore_params_t *) */
