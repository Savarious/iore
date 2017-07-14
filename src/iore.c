#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
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
static void setup_perf_collectors (int);
static void setup_io (access_t, iore_params_t *, iore_offset_t **, void **);
static void cleanup_io (iore_offset_t **, void **);
static void bind_aio_backend (char *);
static void setup_data_signature ();
static char *get_test_file_name (iore_params_t *, access_t, int);
static char *create_rank_dir (char *, int);
static void remove_file (iore_params_t *);
static iore_size_t perform_io (void *, access_t, iore_offset_t *, iore_size_t *,
			       iore_params_t *);
static void delay_secs (int);
static iore_offset_t *get_sequential_offsets (int, iore_params_t *);
static iore_offset_t *get_random_offsets (int, iore_params_t *);
static void *get_buffer (access_t, int);
static int get_pretend_rank (iore_params_t *, access_t);

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
#ifdef USE_MPIIO_AIO
  &iore_aio_mpiio,
#endif
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
  
  /* only tasks participating in this run */
  if (task->comm != MPI_COMM_NULL)
    {
      task->run_id = run->id;
      deadline = current_time() + params->run_time_limit;
      display_run_info(run->id, params);

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
      remove_file (params);
      task->timer[D_STOP][r] = current_time();
      /* TODO: remove rank dirs */

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
  iore_offset_t *offsets = NULL;
  void *buf = NULL;
  void *fd;

  file_name = get_test_file_name (params, WRITE, r);
  if (task->verbosity >= VERY_VERBOSE)
    INFOF("Task %d writing to file %s\n", task->rank, file_name);
  task->test_file_name = file_name;

  delay_secs (params->inter_test_delay);

  if (!params->use_existing_file)
    remove_file (params);

  setup_io (WRITE, params, &offsets, &buf);

  MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  /* create and open the test file */
  task->timer[W_OPEN_START][r] = current_time ();
  fd = task->aio_backend->create (params);
  task->timer[W_OPEN_STOP][r] = current_time ();

  if (params->intra_test_barrier)
    MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  if (task->rank == MASTER_RANK && task->verbosity >= VERBOSE)
    INFOF("Starting write performance test: %s", current_time_str ());

  /* write file */
  task->timer[W_START][r] = current_time ();
  task->data_moved[WRITE][r] = perform_io (fd, WRITE, offsets, buf, params);
  task->timer[W_STOP][r] = current_time ();

  if (params->intra_test_barrier)
    MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  /* close the test file */
  task->timer[W_CLOSE_START][r] = current_time();
  task->aio_backend->close (fd, params);
  task->timer[W_CLOSE_STOP][r] = current_time();

  cleanup_io (&offsets, &buf);

  MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  if (task->verbosity >= DEBUG)
    display_per_task_results (WRITE, r);

  display_test_results (WRITE, r);
} /* exec_write_test (int, iore_params_t *) */

/*
 * Execute a read performance test.
 */
static void
exec_read_test (int r, iore_params_t *params)
{
  char *file_name;
  iore_offset_t *offsets = NULL;
  void *buf = NULL;
  void *fd;

  file_name = get_test_file_name (params, READ, r);
  if (task->verbosity >= VERY_VERBOSE)
    INFOF("Task %d reading from file %s\n", task->rank, file_name);
  task->test_file_name = file_name;
  
  delay_secs (params->inter_test_delay);

  setup_io (READ, params, &offsets, &buf);

  MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  /* open the test file */
  task->timer[R_OPEN_START][r] = current_time ();
  fd = task->aio_backend->open (params);
  task->timer[R_OPEN_STOP][r] = current_time ();

  if (params->intra_test_barrier)
    MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  if (task->rank == MASTER_RANK && task->verbosity >= VERBOSE)
    INFOF("Starting read performance test: %s", current_time_str ());

  /* read file */
  task->timer[R_START][r] = current_time ();
  task->data_moved[READ][r] = perform_io (fd, READ, offsets, buf, params);
  task->timer[R_STOP][r] = current_time ();

  if (params->intra_test_barrier)
    MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  /* close the test file */
  task->timer[R_CLOSE_START][r] = current_time();
  task->aio_backend->close (fd, params);
  task->timer[R_CLOSE_STOP][r] = current_time();

  cleanup_io (&offsets, &buf);

  MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed syncing tasks");

  if (task->verbosity >= DEBUG)
    display_per_task_results (READ, r);

  display_test_results (READ, r);
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
      setup_perf_collectors(params->num_repetitions);
      bind_aio_backend(params->api);
    }
} /* setup_run (iore_params_t *) */

/*
 * Reset performance timers to collect results from an experiment run.
 */ 
static void
setup_perf_collectors (int num_repetitions)
{
  int i;
  for (i = 0; i < NUM_TIMERS; i++)
    {
      task->timer[i] = (iore_time_t *)
	malloc(num_repetitions * sizeof(iore_time_t));
      if (task->timer[i] == NULL)
	{
	  FATAL("Failed to setup performance timers");
	  MPI_Abort(MPI_COMM_WORLD, -1);
	}
    }

  task->data_moved[READ] = (iore_size_t *)
    malloc (num_repetitions * sizeof (iore_size_t));
  if (task->data_moved[READ] == NULL)
    {
      FATAL("Failed to setup data moved collector");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  task->data_moved[WRITE] = (iore_size_t *)
    malloc (num_repetitions * sizeof (iore_size_t));
  if (task->data_moved[WRITE] == NULL)
    {
      FATAL("Failed to setup data moved collector");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
} /* setup_perf_collectors (int) */

/*
 * Setup offsets and buffers for read and write tests.
 */
static void
setup_io (access_t access, iore_params_t *params, iore_offset_t **offsets,
	  void **buf)
{
  int pretend_rank;
  int i;

  pretend_rank = get_pretend_rank (params, access);

  i = pretend_rank % params->block_sizes_length;
  task->block_size = params->block_sizes[i];

  i = pretend_rank % params->transfer_sizes_length;
  task->transfer_size = params->transfer_sizes[i];

  if (params->access_pattern == SEQUENTIAL)
    *offsets = get_sequential_offsets (pretend_rank, params);
  else /* RANDOM */
    *offsets = get_random_offsets (pretend_rank, params);

  *buf = get_buffer (access, pretend_rank);
} /* setup_io (access_t, iore_params_t *, iore_offset_t *, void *) */

/*
 * Deallocate the memory used for file offsets and I/O buffers.
 */
static void
cleanup_io (iore_offset_t **offsets, void **buf)
{
  free (*offsets);
  free (*buf);
} /* cleanup_io (iore_offset_t **, void **) */

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
get_test_file_name (iore_params_t *params, access_t access, int r)
{
  char *file_name = (char *) malloc(MAX_STR_LEN * sizeof(char));
  char *tmp = (char *) malloc(MAX_STR_LEN * sizeof(char)); 
  int pretend_rank;
  
  if (params->sharing_policy == SHARED_FILE)
    strcpy (file_name, params->root_file_name);
  else
    { /* FILE_PER_PROCESS */
      pretend_rank = get_pretend_rank (params, access);
      
      if (params->dir_per_file)
	tmp = create_rank_dir (params->root_file_name, pretend_rank);
      else
	strcpy (tmp, params->root_file_name);

      sprintf (file_name, "%s.%08d", tmp, pretend_rank);
    }

  if (params->use_rep_in_file_name)
    {
      sprintf (tmp, "%s.%d", file_name, r);
      strcpy (file_name, tmp);
    }

  free (tmp);

  return (file_name);
} /* get_test_file_name (iore_params_t *, access_t, int) */

/*
 * Create a directory for the tasks and returns adjusted test file name.
 */
static char *
create_rank_dir (char *file_name, int rank)
{
  char *new_file_name;
  char *dir;
  char *file;

  dir = get_parent_path (file_name);
  sprintf (dir, "%s%d", dir, rank);
  
  if (access (dir, F_OK) != 0)
    {
      if (mkdir (dir, S_IRWXU) < 0)
	FATALF("Failed to create directory %s for test file", dir);
    }
  else if (access (dir, R_OK) != 0 || access (dir, W_OK) != 0 ||
	   access (dir, X_OK) != 0)
    {
      FATALF("Invalid permissions in the directory %s of the test file", dir);
    }
  
  file = get_file_name (file_name);

  new_file_name = dir;
  strcat (new_file_name, "/");
  strcat (new_file_name, file);

  return (new_file_name);
} /* create_rank_dir (char *, int) */

/*
 * Remove the test file pointed by the task context.
 */
static void
remove_file (iore_params_t *params)
{
  if (((params->sharing_policy == SHARED_FILE && task->rank == MASTER_RANK) ||
       params->sharing_policy == FILE_PER_PROCESS) &&
      access (task->test_file_name, F_OK) == 0)
    {
      task->aio_backend->delete (params);
    }
} /* remove_file (iore_params_t *) */

/*
 * Perform data transfer requests for read and write operations.
 */
static iore_size_t
perform_io (void *fd, access_t access, iore_offset_t *offsets, iore_size_t *buf,
	    iore_params_t *params)
{
  iore_size_t remaining = task->block_size;
  iore_size_t transferred = 0;
  iore_size_t data_moved = 0;
  iore_size_t size;
  int i = 0;

  while (offsets[i] != -1)
    {
      if (task->verbosity >= DEBUG)
	{
	  if (access == WRITE)
	    INFOF("Task %d writing to offset %lld\n", task->rank, offsets[i]);
	  else
	    INFOF("Task %d reading from offset %lld\n", task->rank, offsets[i]);
	}

      size = task->transfer_size >= remaining ? remaining : task->transfer_size;

      transferred = task->aio_backend->io (fd, buf, size, offsets[i], access,
					   params);
      if (transferred != size)
	{
	  if (access == WRITE)
	    FATAL("Failed to write to file");
	  else
	    FATAL("Failed to read from file");

	  MPI_Abort(MPI_COMM_WORLD, -1);
	}
      else
	{
	  remaining -= size;
	  data_moved += transferred;
	  i++;
	}
    }

  return (data_moved);
} /* perform_io (void *, access_t, iore_offset_t *, iore_size_t *, ...) */

/*
 * Sleep for n seconds.
 */
static void
delay_secs (int n)
{
  if (task->rank == MASTER_RANK && n > 0)
    {
      if (task->verbosity >= VERBOSE)
	INFOF("Delaying for %d seconds...\n", n);

      sleep (n);
    }
} /* delay_secs (int) */

/*
 * Returns an array of sequential offsets.
 */
static iore_offset_t *
get_sequential_offsets (int rank, iore_params_t *params)
{
  iore_offset_t *offsets;
  iore_offset_t first = 0;
  int n, i, q, r;

  /* count the number of offsets */
  if (task->block_size % task->transfer_size == 0)
    n = task->block_size / task->transfer_size;
  else
    n = (task->block_size / task->transfer_size) + 1;

  /* setup empty array of offsets */
  offsets = (iore_offset_t *) malloc ((n + 1) * sizeof(iore_offset_t));
  if (offsets == NULL)
    FATAL("Failed to setup the array of offsets");

  /* mark the end of the offsets */
  offsets[n] = -1;

  /* fill with file offsets */
  if (params->sharing_policy == FILE_PER_PROCESS)
    {
      for (i = 0; i < n; i++)
	offsets[i] = i * task->transfer_size;
    }
  else /* SHARED_FILE */
    {
      /* compute the first offset of the rank; q is the integer number of loops
	 over all block sizes */
      q = rank / params->block_sizes_length;
      if (q > 0)
	{
	  for (i = 0; i < params->block_sizes_length; i++)
	    first += params->block_sizes[i];

	  first *= q;
	}
      /* r is the remainder loops over block sizes */
      r = rank % params->block_sizes_length;
      for (i = 0; i < r; i++)
	first += params->block_sizes[i];

      /* fill with file offsets */
      for (i = 0; i < n; i++)
	offsets[i] = first + (i * task->transfer_size);
    }
  
  return (offsets);
} /* get_sequential_offsets (int, iore_params_t *) */

/*
 * Returns an array of random offsets.
 */
static iore_offset_t *
get_random_offsets (int rank, iore_params_t *params)
{
  iore_offset_t *offsets;
  iore_offset_t o;
  iore_size_t *remaining;
  iore_size_t file_size = 0;
  iore_size_t transfer_size = 0;
  int n, i, j;

  /* count the number of offsets */
  if (task->block_size % task->transfer_size == 0)
    n = task->block_size / task->transfer_size;
  else
    n = (task->block_size / task->transfer_size) + 1;

  /* setup empty array of offsets */
  offsets = (iore_offset_t *) malloc ((n + 1) * sizeof(iore_offset_t));
  if (offsets == NULL)
    FATAL("Failed to setup the array of offsets");

  /* mark the end of the offsets */
  offsets[n] = -1;

  /* fill with file offsets */
  if (params->sharing_policy == FILE_PER_PROCESS)
    {
      for (i = 0; i < n; i++)
	offsets[i] = i * task->transfer_size;
    }
  else /* SHARED_FILE */
    {
      /* array to guarantee all tasks have their portion of the file */
      remaining = (iore_size_t *) malloc (params->num_tasks *
					  sizeof (iore_size_t));
      for (i = 0; i < params->num_tasks; i++)
	{
	  remaining[i] = params->block_sizes[i % params->block_sizes_length];
	  file_size += remaining[i];
	}

      j = 0;
      for (o = 0; remaining[rank] > 0 && o < file_size; o += transfer_size)
	{
	  do
	    i = random () % params->num_tasks;
	  while (remaining[i] == 0);

	  if (i == rank)
	    offsets[j++] = 0;

	  transfer_size =
	    params->transfer_sizes[i % params->transfer_sizes_length];
	  transfer_size = (transfer_size >= remaining[i] ?
			   remaining[i] : transfer_size);
	  remaining[i] -= transfer_size;
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
} /* get_random_offsets (int, iore_params_t *) */

/*
 * Setup the buffer for read and write tests.
 */
static void *
get_buffer (access_t access, int rank)
{
  unsigned long long *buf;
  unsigned long long even, odd;
  size_t i;
  
  buf = malloc (task->transfer_size);
  if (buf == NULL)
    FATAL("Failed to allocate memory for the I/O buffer");

  if (access == WRITE)
    {
      /* fill buffer */
      even = (unsigned long long) rank;
      odd = task->data_signature;

      for (i = 0; i < (task->transfer_size / sizeof(unsigned long long)); i++)
	buf[i] = (i % 2) == 0 ? even : odd;
    }

  return (buf);
} /* get_buffer (access_t, int) */

/*
 * Returns an alternative rank for read tests, so a task can read a file not in
 * its own node's cache.
 */
static int
get_pretend_rank (iore_params_t *params, access_t access)
{
  int pretend_rank;

  if (access == WRITE || !params->reorder_tasks)
    pretend_rank = task->rank;
  else /* READ and reorder_tasks */
    pretend_rank =
      (task->rank + params->reorder_tasks_offset) % params->num_tasks;

  return (pretend_rank);
} /* get_pretend_rank (iore_params_t *, access_t access) */
