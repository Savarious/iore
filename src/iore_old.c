#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>

#include "ioretypes.h"
#include "util.h"
#include "ioreaio.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

static void
usage (char **);
static void
display_splash ();
static void
display_header (int, char **, enum VERBOSE);
static void
display_test_info (IORE_param_t *);
static void
display_setup (IORE_param_t *);
static void
display_summary (IORE_test_t *);
static void
display_footer ();

static IORE_test_t *
setup_tests (int, char **);
static void
setup_mpi_comm (IORE_param_t *);

static double
get_timestamp ();
static void
check_global_clock ();
static int
has_passed_deadline (int, double);
static void
delay_secs (int, enum VERBOSE);

static IORE_aio_t *
get_aio_backend (char *);
static void *
get_test_file_name (char *, IORE_param_t *);

static IORE_size_t
perform_io (IORE_param_t *, void *, enum ACCESS);
static void
remove_file (char *, IORE_aio_t *, IORE_param_t *);

static void
run (IORE_test_t *);
static void
run_write (IORE_param_t *, IORE_aio_t *, IORE_results_t *, double **, int);
static void
run_read (IORE_param_t *, IORE_aio_t *, IORE_results_t *, double **, int);
static void
finalize (IORE_test_t *);
static void
finalize_iteration (IORE_param_t *, IORE_aio_t *, IORE_results_t *, double **,
		    int);

/*****************************************************************************
 * V A R I A B L E S                                                         *
 *****************************************************************************/

static int nprocs; /* number of MPI ranks */
static int rank; /* MPI rank */
static int error_count = 0;
static double clock_deviation;
static double clock_delta = 0;

/* TODO: use #ifdef to check compiled interfaces */
static IORE_aio_t *available_ioreaio[] =
  { &ioreaio_posix, NULL };

/*****************************************************************************
 * M A I N                                                                   *
 *****************************************************************************/

int
main (int argc, char **argv)
{
  IORE_test_t *tests;

  /* check for the -h or --help options (display usage) in the command
   line before starting MPI. */
  int i;
  for (i = 1; i < argc; i++)
    {
      if (STREQUAL(argv[i], "-h") || STREQUAL(argv[i], "--help"))
	{
	  usage (argv);
	  exit (EXIT_SUCCESS);
	}
    }

  display_splash ();

  /* check for compiled I/O backend */
  if (available_ioreaio[0] == NULL)
    {
      FATAL("No I/O backends compiled for IORE.");
    }

  IORE_MPI_CHECK(MPI_Init (&argc, &argv), "Cannot initialize MPI");
  IORE_MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nprocs),
		 "Cannot get the number of MPI ranks");
  IORE_MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank),
		 "Cannot get the MPI rank");

  /* setup tests based on command line arguments */
  tests = setup_tests (argc, argv);

  display_header (argc, argv, tests->params.verbose);

  /* perform each test */
  IORE_test_t *test;
  for (test = tests; test != NULL; test = test->next)
    {
      if (rank == MASTER_RANK)
	{
	  display_test_info (&test->params);
	}

      run (test);
    }

  display_summary (tests);

  display_footer ();

  finalize (tests);

  IORE_MPI_CHECK(MPI_Finalize (), "Cannot finalize MPI");

  return (error_count);
}

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Displays command line options and help.
 * 
 * **argv: command line arguments
 */
static void
usage (char **argv)
{
  char *opts[] =
    { "OPTIONS:", " -h    display command line options and help", " ",
	"* NOTE: S is a string, N is an integer number.", " ", "" };

  fprintf (stdout, "Usage: %s [OPTIONS]\n\n", *argv);
  int i;
  for (i = 0; strlen (opts[i]) > 0; i++)
    {
      fprintf (stdout, "%s\n", opts[i]);
    }

  return;
}

/*
 * Displays the splash screen.
 */
static void
display_splash ()
{
  /* TODO: include META_VERSION definition */
  fprintf (stdout, "IORE %s - The IOR-Extended Benchmark\n\n", "TBA");
  fflush (stdout);
}

/*
 * Displays the initial header (e.g., command line used, machine name, etc.).
 *
 * argc: number of arguments
 * argv: array of arguments
 * verbose: verbosity level
 */
static void
display_header (int argc, char **argv, enum VERBOSE verbose)
{
  /* TODO: implement */
}

/*
 * Displays the test information.
 *
 * params: test parameters
 */
static void
display_test_info (IORE_param_t *params)
{
  /* TODO: implement */
}

/*
 * Displays the test parameters.
 *
 * params: test parameters.
 */
static void
display_setup (IORE_param_t *params)
{
  /* TODO: implement */
}

/*
 * Displays a summary of all tests.
 *
 * tests: tests list
 */
static void
display_summary (IORE_test_t *tests)
{
  /* TODO: implement */
}

/*
 * Displays concluding information.
 */
static void
display_footer ()
{
  /* TODO: implement */
}

/*
 * Setup tests based on command line arguments.
 *
 * argc: number of arguments
 * argv: array of arguments
 */
static IORE_test_t *
setup_tests (int argc, char **argv)
{
  /* TODO: consider possibility of specifying multiple tests */
  IORE_test_t *tests;

  tests = parse_cmd_line (argc, argv);

  check_global_clock ();

  /* TODO: implement */

  return (tests);
}

/*
 * Setup the MPI communicator for tests.
 *
 * params: test parameters.
 */
static void
setup_mpi_comm (IORE_param_t *params)
{
  int range[3];
  MPI_Group orig_group;
  MPI_Group new_group;
  MPI_Comm comm;

  if (params->num_tasks > nprocs)
    {
      if (rank == MASTER_RANK)
	{
	  WARNF(
	      "More tasks requested (%d) than available (%d); using %d tasks.\n",
	      params->num_tasks, nprocs, nprocs);
	}
      params->num_tasks = nprocs;
    }

  range[0] = 0; /* first rank */
  range[1] = params->num_tasks - 1; /* last rank */
  range[2] = 1; /* stride */

  IORE_MPI_CHECK(MPI_Comm_group(MPI_COMM_WORLD, &orig_group),
		 "Failed to get original MPI group.");
  IORE_MPI_CHECK(MPI_Group_range_incl (orig_group, 1, &range, &new_group),
		 "Failed to create new MPI group.");
  IORE_MPI_CHECK(MPI_Comm_create(MPI_COMM_WORLD, new_group, &comm),
		 "Failed to create new MPI communicator.");
  params->comm = comm;

  /* sychronize tasks not participating in this test */
  if (comm == MPI_COMM_NULL)
    {
      IORE_MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD),
		     "Failed to block unnused tasks");
    }
}

/*
 * Get current timestamp.
 */
static double
get_timestamp ()
{
  double t;

  t = MPI_Wtime ();
  if (t < 0)
    {
      FATAL("Failed to get timestamp using MPI_Wtime().");
    }
  t -= clock_delta;

  return t;
}

/*
 * Get differences in times between nodes.
 */
static void
check_global_clock ()
{
  double ts;
  double master_ts;
  double min = 0;
  double max = 0;

  /* block all tasks */
  IORE_MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD), "Failed to barrier");

  ts = get_timestamp ();

  /* reduce to get min and max timestamps */
  IORE_MPI_CHECK(
      MPI_Reduce(&ts, &min, 1, MPI_DOUBLE, MPI_MIN, MASTER_RANK, MPI_COMM_WORLD),
      "Failed to reduce tasks' timestamps");
  IORE_MPI_CHECK(
      MPI_Reduce(&ts, &max, 1, MPI_DOUBLE, MPI_MAX, MASTER_RANK, MPI_COMM_WORLD),
      "Failed to reduce tasks' timestamps");

  /* compute clock delta in nodes considering the master rank */
  master_ts = ts;
  IORE_MPI_CHECK(
      MPI_Bcast(&master_ts, 1, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD),
      "Failed to broadcast master timestamp");
  clock_delta = ts - master_ts;

  /* compute total clock deviation across all nodes */
  clock_deviation = max - min;

  return;
}

/*
 * Check whether test deadline has been passed.
 *
 * max_time: max time in seconds for a test.
 * start_time: test start time.
 */
static int
has_passed_deadline (int max_time, double start_time)
{
  double deadline;
  int has_passed = FALSE;

  if (max_time > 0)
    {
      deadline = start_time + max_time;
      has_passed = get_timestamp () >= deadline;
    }

  return has_passed;
}

/*
 * Sleep for t seconds.
 *
 * t: number of seconds.
 * verbose: verbosity level.
 */
static void
delay_secs (int t, enum VERBOSE verbose)
{
  if (rank == MASTER_RANK && t > 0)
    {
      if (verbose >= VERBOSE_1)
	{
	  INFOF("Delaying for %d seconds...\n", t);
	  sleep (t);
	}
    }
}

/*
 * Get the backend for the abstract I/O.
 *
 * api: name of the API
 */
static IORE_aio_t *
get_aio_backend (char *api)
{
  IORE_aio_t *backend = NULL;
  IORE_aio_t **ioreaio = available_ioreaio;

  while (*ioreaio != NULL && backend == NULL)
    {
      if (STREQUAL(api, (*ioreaio)->name))
	{
	  backend = *ioreaio;
	}
      ioreaio++;
    }

  if (backend == NULL)
    {
      FATAL("Unrecognized I/O API.");
    }
}

/*
 * TODO: document
 */
static void *
get_test_file_name (char *file_name, IORE_param_t *params)
{
  /* TODO: implement */
}

/*
 * TODO: document
 */
static IORE_size_t
perform_io (IORE_param_t *params, void *fd, enum ACCESS access)
{
  /* TODO: consider randomOffset for read tests */



  /* TODO: continue... */
}

/*
 * Remove the file(s).
 *
 * file_name: full name of the file.
 * backend: I/O backend.
 * params: test parameters.
 */
static void
remove_file (char *file_name, IORE_aio_t *backend, IORE_param_t *params)
{
  if (params->sharing_policy == SHARED_FILE)
    {
      if (rank == MASTER_RANK && access (file_name, F_OK) == 0)
	{
	  backend->delete (file_name, MASTER_RANK, params);
	}
    }
  else
    { /* FILE_PER_PROCESS */
      /* TODO: implement */
    }
}

/*
 * Execute iterations of a single test.
 */
static void
run (IORE_test_t *test)
{
  IORE_aio_t *backend;
  IORE_param_t *params = &test->params;
  enum VERBOSE verbose = params->verbose;
  double *timer[NUM_TIMERS];
  double start_time;
  int max_time = params->max_time_per_test;

  setup_mpi_comm (params);
  if (rank == MASTER_RANK && verbose >= VERBOSE_1)
    {
      INFOF("Participating tasks: %d\n", params->num_tasks);
    }

  /* TODO: check count_tasks_per_node need */

  /* setup timers */
  int i;
  for (i = 0; i < NUM_TIMERS; i++)
    {
      timer[i] = (double *) malloc (params->repetitions * sizeof(double));
      if (timer[i] == NULL)
	{
	  FATAL("Failed to allocated memory for timers.");
	}
    }

  backend = get_aio_backend (params->api);

  if (rank == MASTER_RANK && verbose >= VERBOSE_0)
    {
      display_setup (params);
    }

  /* TODO: check hogMemory need */

  start_time = get_timestamp ();

  /* loop over test repetitions */
  int r;
  for (r = 0; r < params->repetitions; r++)
    {
      /* TODO: check pre-operation statements */

      /* write test */
      if (params->write && !has_passed_deadline (max_time, start_time))
	{
	  run_write (params, backend, test->results, timer, r);
	} /* end of write test */

      /* check write test */
      if (params->write_check && !has_passed_deadline (max_time, start_time))
	{
	  /* TODO: implement */
	} /* end of check write test */

      /* read test */
      if (params->read && !has_passed_deadline (max_time, start_time))
	{
	  run_read (params, backend, test->results, timer, r);
	} /* end of read test */

      /* check read test */
      if (params->read_check && !has_passed_deadline (max_time, start_time))
	{
	  /* TODO: implement */
	} /* end of check read test */

      /* synchronize tasks */
      IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");

      /* finalize test iteration */
      finalize_iteration (params, backend, test->results, timer, r);
    } /* end of loop over test iterations */

  /* TODO: display summary of results of the test */

  /* clean-up */
  for (i = 0; i < NUM_TIMERS; i++)
    {
      free (timer[i]);
    }

  IORE_MPI_CHECK(MPI_Comm_free (&params->comm),
		 "Failed to free the MPI communicator.");

  /* synchronize with tasks not participating in this test */
  IORE_MPI_CHECK(MPI_Barrier (MPI_COMM_WORLD), "Failed to synchronize tasks");
}

/*
 * Execute a single iteration of a write test.
 *
 * params: test parameters.
 * backend: I/O backend.
 * results: structure with performance test measures.
 * timer: performance timers.
 * r: repetition number.
 */
static void
run_write (IORE_param_t *params, IORE_aio_t *backend, IORE_results_t *results,
	   double **timer, int r)
{
  enum VERBOSE verbose = params->verbose;
  char *file_name;
  void *fd;
  IORE_size_t data_moved;

  /* define file name */
  get_test_file_name (file_name, params);
  if (verbose >= VERBOSE_3)
    {
      INFOF("Task %d writing %s\n", rank, file_name);
    }

  /* delay between tests */
  delay_secs (params->intra_test_delay, verbose);

  /* define initial file condition */
  if (!params->use_existing_test_file)
    {
      remove_file (file_name, backend, params);
    }

  /* synchornize tasks */
  IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");

  /* TODO: check need for open and openFlags attrs related to HDF5 */

  /* create/open file */
  timer[W_OPEN_START][r] = get_timestamp ();
  fd = backend->create (file_name, params);
  timer[W_OPEN_STOP][r] = get_timestamp ();

  if (params->intra_test_sync)
    {
      IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");
    }

  if (rank == MASTER_RANK && verbose >= VERBOSE_1)
    {
      INFOF("Starting write performance test: %s", get_time_string ());
    }

  /* write file */
  timer[W_START][r] = get_timestamp ();
  data_moved = perform_io (params, fd, WRITE);
  timer[W_STOP][r] = get_timestamp ();

  if (params->intra_test_sync)
    {
      IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");
    }

  /* close file */
  timer[W_CLOSE_START][r] = get_timestamp ();
  backend->close (fd, params);
  timer[W_CLOSE_STOP][r] = get_timestamp ();

  /* synchornize tasks */
  IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");

  /* TODO: check need for collecting the aggregate file size */

  display_test_results ();
  /* TODO: implement */
}

/*
 * TODO: document
 */
static void
run_read (IORE_param_t *params, IORE_aio_t *backend, IORE_results_t *results,
	  double **timer, int repetition)
{
  /* TODO: implement */
}

/*
 * Execute finalizing actions (e.g., destroy tests, etc.).
 */
static void
finalize (IORE_test_t *test)
{
  /* TODO: implement */
}

/*
 * TODO: document
 */
static void
finalize_iteration (IORE_param_t *params, IORE_aio_t *backend,
		    IORE_results_t *results, double **timer, int r)
{
  if (!params->keep_file
      && !(params->error_found && params->keep_file_with_error))
    {
      timer[D_START][r] = get_timestamp ();
      remove_file (file_name, backend, params);
      timer[D_STOP][r] = get_timestamp ();

      /* synchronize tasks */
      IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");

      display_test_results ();
    }

  params->error_found = FALSE;
}
