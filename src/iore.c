#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ioretypes.h"
#include "util.h"
#include "ioreaio.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

static void
handle_preemptive_args (int, char**);

static void
display_version ();
static void
display_usage (char **);
static void
display_splash ();
static void
display_header ();

static void
display_summary (IORE_run_t *);
static void
display_test_results ();

static IORE_run_t *
setup_experiment (int, char**);
static void
cleanup_experiment (IORE_run_t *);

static void
setup_run (IORE_params_t *);
static void
cleanup_run (IORE_params_t *);
static void
setup_mpi_comm (IORE_params_t *);
static void
setup_timers (int);
static void
setup_aio_backend (char *);
static void
setup_io (enum ACCESS_TYPE, IORE_params_t *, IORE_offset_t *, void *);
static void
cleanup_io (IORE_offset_t *, void *);
static void
setup_buffer (enum ACCESS_TYPE, int, IORE_params_t *, void **);

static void
exec_run (IORE_run_t *);
static void
exec_write_test (IORE_params_t *, IORE_results_t *, int);
static void
exec_read_test (IORE_params_t *, IORE_results_t *, int);

static double
get_timestamp ();
static int
is_past_deadline ();
static void
delay_secs (int);

static void
remove_file (IORE_params_t *);
static IORE_size_t
perform_io (void *, enum ACCESS_TYPE, IORE_offset_t *, IORE_size_t *,
	    IORE_params_t *);

static char *
get_file_name (IORE_params_t *);
static char *
prepend_dir (char *, int);
static IORE_offset_t *
get_sequential_offsets (int, IORE_params_t *);
static IORE_offset_t *
get_random_offsets (int, IORE_params_t *);
static int
get_random_seed (MPI_Comm);

/*****************************************************************************
 * D E C L A R A T I O N S                                                   *
 *****************************************************************************/

static int nprocs; /* number of MPI tasks */
int rank; /* this task MPI rank */
static int rank_offset; /* an offset in number of MPI ranks */
static enum VERBOSITY verbose; /* verbosity level */

static IORE_time_t *timer[NUM_TIMERS]; /* timers for each experiment run */
static double clock_delta = 0; /* time difference to the master rank clock */

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
static void
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
    display_version ();

  if (h)
    display_usage (argv);

  if (v || h)
    exit (EXIT_SUCCESS);
} /* handle_preemptive_args (int, char**) */

/*
 * Execute all replications of a single run of the experiment.
 */
static void
exec_run (IORE_run_t *run)
{
  IORE_params_t *params = &run->params;
  IORE_results_t *results = &run->results;
  int time_limit = params->run_time_limit;
  double start_time;
  int r;

  setup_run (params);

  /* only tasks participating in this run */
  if (params->comm != MPI_COMM_NULL)
    {
      start_time = get_timestamp ();

      /* loop over run replications */
      for (r = 0; r < params->num_repetitions; r++)
	{
	  params->repetition = r;

	  if (params->write_test && !is_past_deadline (time_limit, start_time))
	    exec_write_test (params, results, r);

	  if (params->read_test && !is_past_deadline (time_limit, start_time))
	    exec_read_test (params, results, r);

	  if (!params->keep_file)
	    {
	      timer[D_START][r] = get_timestamp ();
	      remove_file (params);
	      timer[D_STOP][r] = get_timestamp ();

	      /* synchronize participating tasks */
	      IORE_MPI_CHECK(MPI_Barrier (params->comm),
			     "Failed to synchronize tasks.");

	      /* TODO: display remove results */
	    }
	}
    }

  cleanup_run (params);

  /* synchronize all tasks */
  IORE_MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD), "Failed to synchronize tasks.");
} /* exec_run (IORE_run_t *) */

/*
 * Execute a single write performance test.
 */
static void
exec_write_test (IORE_params_t *params, IORE_results_t *results, int r)
{
  IORE_size_t data_moved = 0;
  char *file_name;
  void *fd;
  IORE_offset_t *offsets;
  void *buf;

  file_name = get_file_name (params);
  if (verbose >= TASK)
    INFOF("Task %d writing to %s\n", rank, file_name);

  delay_secs (params->inter_test_delay);

  if (!params->use_existing_file)
    remove_file (params);

  setup_io (WRITE, params, offsets, buf);

  /* synchronize participating tasks */
  IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks.");

  /* create and open file */
  timer[W_OPEN_START][r] = get_timestamp ();
  fd = backend->create (params);
  timer[W_OPEN_STOP][r] = get_timestamp ();

  if (params->intra_test_barrier)
    IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks.");

  if (rank == MASTER_RANK && verbose >= CONTROL)
    INFOF("Starting write performance test: %s", get_time_string ());

  /* write file */
  timer[W_START][r] = get_timestamp ();
  data_moved = perform_io (fd, WRITE, offsets, buf, params);
  timer[W_STOP][r] = get_timestamp ();

  if (params->intra_test_barrier)
    IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks.");

  /* close file */
  timer[W_CLOSE_START][r] = get_timestamp ();
  backend->close (fd, params);
  timer[W_CLOSE_STOP][r] = get_timestamp ();

  cleanup_io(offsets, buf);

  /* synchornize participating tasks */
  IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");

  display_test_results ();
} /* exec_write_test (IORE_params_t *, IORE_results_t *, int) */

/*
 * Execute a single read performance test.
 */
static void
exec_read_test (IORE_params_t *params, IORE_results_t *results, int r)
{
  char *file_name;
  void *fd;
  IORE_size_t data_moved;
  IORE_offset_t *offsets;
  void *buf;

  if (params->reorder_tasks)
    rank_offset = (params->reorder_tasks_offset * params->tasks_per_node)
	% params->eff_num_tasks;

  file_name = get_file_name (params);
  if (verbose >= TASK)
    INFOF("Task %d reading from %s\n", rank, file_name);

  delay_secs (params->inter_test_delay);

  setup_io (READ, params, offsets, buf);

  /* synchronize participating tasks */
  IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks.");

  /* open file */
  timer[R_OPEN_START][r] = get_timestamp ();
  fd = backend->open (params);
  timer[R_OPEN_STOP][r] = get_timestamp ();

  if (params->intra_test_barrier)
    IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks.");

  if (rank == MASTER_RANK && verbose >= CONTROL)
    INFOF("Starting read performance test: %s", get_time_string ());

  /* read file */
  timer[R_START][r] = get_timestamp ();
  data_moved = perform_io (fd, READ, offsets, buf, params);
  timer[R_STOP][r] = get_timestamp ();

  if (params->intra_test_barrier)
    IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks.");

  /* close file */
  timer[R_CLOSE_START][r] = get_timestamp ();
  backend->close (fd, params);
  timer[R_CLOSE_STOP][r] = get_timestamp ();

  cleanup_io(offsets, buf);

  /* synchornize participating tasks */
  IORE_MPI_CHECK(MPI_Barrier (params->comm), "Failed to synchronize tasks");

  display_test_results ();
} /* exec_read_test (IORE_params_t *, IORE_results_t *, int) */

/*
 * Setup parameters for a single run of the experiment.
 */
static void
setup_run (IORE_params_t *params)
{
  time_t curtime;

  verbose = params->verbose;

  setup_mpi_comm (params);
  /* only tasks participating in this run */
  if (params->comm != MPI_COMM_NULL)
    {
      if (rank == MASTER_RANK && verbose >= CONTROL)
	INFOF("Participating tasks: %d\n", params->eff_num_tasks);

      setup_timers (params->num_repetitions);

      setup_aio_backend (params->api);

      curtime = time (NULL);
      if (curtime == -1)
	FATAL("Failed to get current timestamp.");
      params->timestamp_signature = curtime;
    }
} /* setup_run (IORE_params_t *) */

/*
 * Clean-up variables of an experiment run.
 */
static void
cleanup_run (IORE_params_t *params)
{
  int i;

  for (i = 0; i < NUM_TIMERS; i++)
    free (timer[i]);

  IORE_MPI_CHECK(MPI_Comm_free (&params->comm),
		 "Failed to free the MPI communicator.");
} /* cleanup_run (IORE_params_t *) */

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
  int num_tasks = params->num_tasks;

  /* check if there are enough tasks in the MPI communicator;
   * otherwise execute with the available number of tasks */
  if (num_tasks > nprocs)
    {
      if (rank == MASTER_RANK)
	WARNF("More tasks requested (%d) than available (%d); "
	      "running on %d tasks.\n",
	      num_tasks, nprocs, num_tasks);
      params->eff_num_tasks = nprocs;
    }
  else
    {
      params->eff_num_tasks = num_tasks;
    }

  /* create new MPI communicator */
  IORE_MPI_CHECK(MPI_Comm_group(MPI_COMM_WORLD, &group),
		 "Failed to get MPI group.");
  range[0] = 0; /* first rank */
  range[1] = params->eff_num_tasks - 1; /* last rank */
  range[2] = 1; /* stride */
  IORE_MPI_CHECK(MPI_Group_range_incl (group, 1, &range, &newgroup),
		 "Failed to define new MPI group.");
  IORE_MPI_CHECK(MPI_Comm_create(MPI_COMM_WORLD, newgroup, &comm),
		 "Failed to create the new MPI communicator.");

  params->comm = comm;
} /* setup_mpi_comm (IORE_params_t *) */

/*
 * Setup the timers used to register test performance.
 */
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

/*
 * Setup the abstract I/O implementation.
 */
static void
setup_aio_backend (char *api)
{
  IORE_aio_t **aio = available_aio;

  backend = NULL;
  while (*aio != NULL && backend == NULL)
    {
      if (STREQUAL(api, (*aio)->name))
	backend = *aio;
      aio++;
    }

  if (backend == NULL)
    FATAL("Unrecognized abstract I/O API.");
} /* setup_aio_backend (char *) */

/*
 * Setup buffers and offsets for I/O tests.
 */
static void
setup_io (enum ACCESS_TYPE access, IORE_params_t *params,
	  IORE_offset_t *offsets, void *buf)
{
  int pretend_rank;
  int i;

  pretend_rank = (rank + rank_offset) % params->eff_num_tasks;

  i = pretend_rank % ARRLENGTH(params->block_sizes);
  params->block_size = params->block_sizes[i];

  i = pretend_rank % ARRLENGTH(params->transfer_sizes);
  params->transfer_size = params->transfer_sizes[i];

  if (params->access_pattern == SEQUENTIAL)
    offsets = get_sequential_offsets (pretend_rank, params);
  else
    /* RANDOM */
    offsets = get_random_offsets (pretend_rank, params);

  setup_buffer (access, pretend_rank, params, &buf);
} /* setup_io (enum ACCESS_TYPE, IORE_params_t *, IORE_offset_t *, void *) */

/*
 * Free memory used for I/O buffers and file offsets.
 */
static void
cleanup_io (IORE_offset_t *offsets, void *buf)
{
  free (offsets);
  free (buf);
} /* cleanup_io (IORE_offset_t *, void *) */

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
} /* get_timestamp () */

/*
 * Check whether the run time deadline is past.
 */
static int
is_past_deadline (int time_limit, double start_time)
{
  double deadline;
  int is_past = FALSE;

  if (time_limit > 0)
    {
      deadline = start_time + time_limit;
      is_past = get_timestamp () >= deadline;
    }

  return is_past;
} /* is_past_deadline (int, double) */

/*
 * Sleep for n seconds.
 */
static void
delay_secs (int n)
{
  if (rank == MASTER_RANK && n > 0)
    {
      if (verbose >= CONTROL)
	INFOF("Delaying for %d seconds...\n", n);

      sleep (n);
    }
} /* delay_secs (int) */

/*
 * Remove test files.
 */
static void
remove_file (IORE_params_t *params)
{
  char *file_name = params->file_name;

  if (((params->sharing_policy == SHARED_FILE && rank == MASTER_RANK)
      || params->sharing_policy == FILE_PER_PROCESS)
      && access (file_name, F_OK == 0))
    {
      backend->delete (params);
    }
} /* remove_file (IORE_params_t *) */

/*
 * Read/Write data from/to test file(s).
 */
static IORE_size_t
perform_io (void *fd, enum ACCESS_TYPE access, IORE_offset_t *offsets,
	    IORE_size_t *buf, IORE_params_t *params)
{
  IORE_size_t remaining = params->block_size;
  IORE_size_t transfer_size = params->transfer_size;
  IORE_size_t data_moved = 0;
  IORE_size_t total_data_moved = 0;
  IORE_size_t tx;
  int i = 0;

  while (offsets[i] != -1)
    {
      params->offset = offsets[i];

      tx = (transfer_size >= remaining ? remaining : transfer_size);

      data_moved = backend->io (fd, buf, tx, access, params);
      if (data_moved != params->transfer_size)
	FATALF("Failed to %s %s file.", access == WRITE ? "write" : "read",
	       access == WRITE ? "to" : "from");

      remaining -= tx;
      total_data_moved += data_moved;
      i++;
    }

  return (total_data_moved);
} /* perform_io (void *, enum ACCESS_TYPE, IORE_params_t *, ...) */

/*
 * Generate the actual name of the file that will be accessed in the test.
 */
static char *
get_file_name (IORE_params_t *params)
{
  char file_name[MAXPATHLEN];
  char affix[MAX_STR];

  if (params->sharing_policy == SHARED_FILE)
    {
      strcpy (file_name, params->file_name);
    }
  else
    { /* FILE_PER_PROCESS */
      if (params->dir_per_file)
	strcpy (affix, prepend_dir (params->file_name, params->eff_num_tasks));

      sprintf (file_name, "%s.%08d", params->file_name,
	       (rank + rank_offset) % params->eff_num_tasks);
    }

  if (params->rep_in_file_name)
    {
      sprintf (affix, ".%d", params->repetition);
      strcat (file_name, affix);
    }

  return (file_name);
} /* get_file_name(IORE_params_t *) */

/*
 * Create a subdirectory with rank as name as the parent of the test file.
 */
static char *
prepend_dir (char *file_name, int num_tasks)
{
  char dir[MAXPATHLEN];
  char file[MAXPATHLEN];
  char *path;
  int i;

  /* get the parent dir path */
  strcpy (dir, file_name);
  i = strlen (dir) - 1;
  while (i > 0)
    {
      if (dir[i] == '\0' || dir[i] == '/')
	{
	  dir[i] = '/';
	  dir[i + 1] = '\0';
	  break;
	}
      i--;
    }

  /* get file name */
  strcpy (file, file_name);
  path = file;
  while (i > 0)
    {
      if (file[i] == '\0' || file[i] == '/')
	{
	  path = file + (i + 1);
	  break;
	}
      i--;
    }

  /* create rank directory */
  sprintf (dir, "%s%d", dir, (rank + rank_offset) % num_tasks);
  if (access (dir, F_OK) != 0)
    {
      if (mkdir (dir, S_IRWXU) < 0)
	FATAL("Failed to create directory for test file.");
    }
  else if (access (dir, R_OK) != 0 || access (dir, W_OK) != 0
      || access (dir, X_OK) != 0)
    {
      FATAL("Invalid permissions in the directory of the test file.");
    }

  strcat (dir, "/");
  strcat (dir, path);

  return (dir);
} /* prepend_dir (char *, int) */

/*
 * Generate an array of sequential file offsets.
 */
static IORE_offset_t *
get_sequential_offsets (int pretend_rank, IORE_params_t *params)
{
  IORE_offset_t *offsets;
  IORE_size_t block_size = params->block_size;
  IORE_size_t transfer_size = params->transfer_size;
  IORE_offset_t first = 0;
  int n, i, l, q, r;

  /* count the number of offsets */
  if (block_size % transfer_size == 0)
    n = block_size / transfer_size;
  else
    n = (block_size / transfer_size) + 1;

  /* setup empty array of offsets */
  offsets = (IORE_offset_t *) malloc ((n + 1) * sizeof(IORE_offset_t));
  if (offsets == NULL)
    FATAL("Failed to setup array of offsets.");

  /* mark the end of the offsets */
  offsets[n] = -1;

  /* fill with file offsets */
  if (params->sharing_policy == FILE_PER_PROCESS)
    {
      for (i = 0; i < n; i++)
	offsets[i] = i * transfer_size;
    }
  else /* SHARED_FILE */
    {
      /* compute the first offset of the pretend rank */
      l = ARRLENGTH(params->block_sizes);
      q = pretend_rank / l; /* number of integer loops over block sizes */
      if (q > 0)
	{
	  for (i = 0; i < l; i++)
	    first += params->block_sizes[i];
	  first *= q;
	}
      r = pretend_rank % l; /* remainder loops over block sizes */
      for (i = 0; i < r; i++)
	first += params->block_sizes[i];

      for (i = 0; i < n; i++)
	offsets[i] = first + i * transfer_size;
    }

  return (offsets);
} /* get_sequential_offsets (int, IORE_params_t *) */

/*
 * Generate an array of random file offsets.
 */
static IORE_offset_t *
get_random_offsets (int pretend_rank, IORE_params_t *params)
{
  IORE_offset_t *offsets;
  IORE_offset_t o;
  IORE_size_t block_size = params->block_size;
  IORE_size_t transfer_size = params->transfer_size;
  IORE_size_t *remaining;
  IORE_size_t file_size = 0;
  IORE_size_t tx;
  int seed;
  int n, i, j;
  int lb, lt;

  /* setup random */
  seed = get_random_seed (params->comm);
  params->rnd_seed = seed;
  srandom (seed);

  /* count the number of offsets */
  if (block_size % transfer_size == 0)
    n = block_size / transfer_size;
  else
    n = (block_size / transfer_size) + 1;

  /* setup empty array of offsets */
  offsets = (IORE_offset_t *) malloc ((n + 1) * sizeof(IORE_offset_t));
  if (offsets == NULL)
    FATAL("Failed to setup array of offsets.");

  /* mark the end of the offsets */
  offsets[n] = -1;

  /* fill with file offsets */
  if (params->sharing_policy == FILE_PER_PROCESS)
    {
      for (i = 0; i < n; i++)
	offsets[i] = i * transfer_size;
    }
  else /* SHARED_FILE */
    {
      j = 0;
      lb = ARRLENGTH(params->block_sizes);
      lt = ARRLENGTH(params->transfer_sizes);
      /* array to guarantee all tasks have its portion of the file */
      remaining = (IORE_size_t *) malloc (
	  params->eff_num_tasks * sizeof(IORE_size_t));
      for (i = 0; i < params->eff_num_tasks; i++)
	{
	  remaining[i] = params->block_sizes[i % lb];
	  file_size += remaining[i];
	}

      for (o = 0; remaining[pretend_rank] > 0 && o < file_size; o += tx)
	{
	  do
	    i = random () % params->eff_num_tasks;
	  while (remaining[i] == 0);

	  if (i == pretend_rank)
	    {
	      offsets[j] = o;
	      j++;
	    }

	  tx = params->transfer_sizes[i % lt];
	  tx = (tx >= remaining[i] ? remaining[i] : tx);
	  remaining[i] -= tx;
	}
    }

  /* shuffle offsets */
  for (i = 0; i < n; i++)
    {
      j = random () % n;
      o = offsets[j];
      offsets[j] = offsets[i];
      offsets[i] = o;
    }

  return (offsets);
} /* get_random_offsets (int, IORE_params_t *) */

/*
 * Setup the I/O buffer used in read/write tests.
 */
static void
setup_buffer (enum ACCESS_TYPE access, int pretend_rank, IORE_params_t *params,
	      void **buffer)
{
  IORE_size_t transfer_size = params->transfer_size;
  size_t i;
  unsigned long long even, odd;
  unsigned long long *buf;

  *buffer = malloc (transfer_size);
  if (buffer == NULL)
    FATAL("Failed to allocate memory for the I/O buffer");

  if (access == WRITE)
    {
      /* fill buffer */
      buf = (unsigned long long *) buffer;
      even = (unsigned long long) pretend_rank;
      odd = (unsigned long long) params->timestamp_signature;

      for (i = 0; i < transfer_size / sizeof(unsigned long long); i++)
	{
	  if ((i % 2) == 0)
	    buf[i] = even;
	  else
	    buf[i] = odd;
	}
    }
} /* setup_buffer (enum ACCESS_TYPE, int, IORE_params_t *, void **) */

/*
 * Generates a random seed, broadcasts it to participating tasks and returns.
 */
static int
get_random_seed (MPI_Comm comm)
{
  unsigned int seed;

  if (rank == MASTER_RANK)
    {
      struct timeval time;
      gettimeofday (&time, (struct timezone *) NULL);
      seed = time.tv_usec;
    }

  IORE_MPI_CHECK(MPI_Bcast(&seed, 1, MPI_INT, 0, comm),
		 "Failed to broadcast the random seed value.");

  return (seed);
} /* get_random_seed (MPI_Comm) */
