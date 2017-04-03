#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* environ, gethostname */
#include <sys/utsname.h> /* uname */
#include <sys/param.h> /* MAXPATHLEN */
#include <sys/statvfs.h>
#include <mpi.h>

#include "display.h"
#include "iore_task.h"
#include "iore_params.h"
#include "util.h"

/*****************************************************************************
 * G L O B A L S
 *****************************************************************************/

extern iore_task_t *task;
extern char **environ;

/*****************************************************************************
 * F U N C T I O N S
 *****************************************************************************/

/*
 * Shows the program name and version.
 */
void
display_version()
{
  if (task->rank == MASTER_RANK)
    {
      fprintf(stdout, "IORE-%s: the IOR-Extended benchmark\n\n", META_VERSION);
      fflush(stdout);
    }
} /* display_version() */

/*
 * Shows program usage options and help message.
 */
void
display_usage(char **argv)
{
  int i;
  char *opts[] = {
    "OPTIONS:",
    "  -f FILE   : path to the file defining the experiment",
    "  --help    : displays this usage help message",
    "  --version : displays program version",
    ""
  };
  
  if (task->rank == MASTER_RANK)
    {
      fprintf(stdout, "Usage: %s [OPTIONS]\n\n", *argv);
      for (i = 0; strlen(opts[i]) > 0; i++)
	fprintf(stdout, "%s\n", opts[i]);
      fprintf(stdout, "\n");
      fflush(stdout);
    }
} /* display_usage(char **) */

/*
 * Shows a splash screen message.
 */
void
display_splash()
{
  if (task->verbosity >= NORMAL)
    display_version();
} /* display_splash() */

/*
 * Shows general information about an experiment.
 */
void
display_expt_header(int argc, char **argv)
{
  struct utsname buf;
  int i;
  
  if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
    {
      fprintf(stdout, "Start: %s", current_time_str());
      fprintf(stdout, "Command line used:");
      for (i = 0; i < argc; i++)
	fprintf(stdout, " %s", argv[i]);
      fprintf(stdout, "\n");

      if (uname(&buf) != 0)
	{
	  WARN("Failed to obtain the host name.");
	  fprintf(stdout, "Machine: UNKNOWN\n");
	}
      else
	{
	  fprintf(stdout, "Machine: %s %s %s %s %s\n", buf.sysname,
		  buf.nodename, buf.release, buf.version, buf.machine);
	}

      if (task->verbosity >= VERBOSE)
	fprintf(stdout, "Max time deviation across all tasks: %.05f sec\n\n",
		task->wclock_skew_all);

      if (task->verbosity >= DEBUG)
	{
	  fprintf(stdout, "Environment variables:\n");
	  for (i = 0; environ[i] != NULL; i++)
	    fprintf(stdout, "%s\n", environ[i]);
	  fprintf(stdout, "\n");
	}
      
      fflush(stdout);
    }
} /* display_expt_header(char **) */

/*
 * Shows information about an experiment run.
 */
void
display_run_info (int id, iore_params_t *params)
{
  char hostname[MAX_STR_LEN];
  
  if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
    {
      fprintf(stdout, "Test %d started: %s\n", id, current_time_str());

      if (task->verbosity >= VERY_VERBOSE)
	display_fs_info(params->root_file_name);

      display_params(params);

      fprintf(stdout, "Participating tasks: %d\n\n", params->num_tasks);
    }

  if (task->verbosity >= VERY_VERBOSE)
    {
      MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed task syncing");

      if (gethostname (hostname, MAX_STR_LEN) == 1)
	{
	  WARNF("Task %d failed to obtain the host name.", task->rank);
	  strcpy(hostname, "UNKNOWN");
	}

      fprintf(stdout, "Task %d on %s\n", task->rank, hostname);
    }
  
  fflush(stdout);

  MPI_TRYCATCH(MPI_Barrier (task->comm), "Failed task syncing");
} /* display_run_info (int, iore_params_t *) */

/*
 * Shows provided parameters at the appropriate verbosity level.
 */
void
display_params (iore_params_t *params)
{
  int i;
  
  if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
    {
      if (task->verbosity >= VERBOSE)
	{
	  fprintf(stdout, "Run parameters:\n");
	  fprintf(stdout, "\t%s = %d\n", "ref_num", params->ref_num);
	}
      else
	fprintf(stdout, "Run summary:\n");

      fprintf(stdout, "\t%s = %d\n", "num_tasks", params->num_tasks);
      fprintf(stdout, "\t%s = %s\n", "api", params->api);
      fprintf(stdout, "\t%s = %s\n", "sharing_policy",
	      (params->sharing_policy == SHARED_FILE ) ?
	      "SHARED_FILE" : "FILE_PER_PROCESS");
      fprintf(stdout, "\t%s = %s\n", "access_pattern",
	      (params->access_pattern == SEQUENTIAL ) ?
	      "SEQUENTIAL" : "RANDOM");

      fprintf(stdout, "\t%s = { %s", "block_sizes",
	      human_readable(params->block_sizes[0], 2));
      for (i = 1; i < params->block_sizes_length; i++)
	fprintf(stdout, ", %s", human_readable(params->block_sizes[i], 2));
      fprintf(stdout, " }\n");

      fprintf(stdout, "\t%s = { %s", "transfer_sizes",
	      human_readable(params->transfer_sizes[0], 2));
      for (i = 1; i < params->transfer_sizes_length; i++)
	fprintf(stdout, ", %s", human_readable(params->transfer_sizes[i], 2));
      fprintf(stdout, " }\n");

      if (params->write_test)
	{
	  fprintf(stdout, "\t%s = write", "test");
	  if (params->read_test)
	    fprintf(stdout, ", read");
	  fprintf(stdout, "\n");
	}
      else if (params->read_test)
	{
	  fprintf(stdout, "\t%s = read\n", "test");
	}

      fprintf(stdout, "\t%s = %s\n", "root_file_name", params->root_file_name);

      if (task->verbosity >= VERBOSE)
	{
	  fprintf(stdout, "\t%s = %d\n", "num_repetitions",
		  params->num_repetitions);
	  fprintf(stdout, "\t%s = %d sec\n", "inter_test_delay",
		  params->inter_test_delay);
	  fprintf(stdout, "\t%s = %s\n", "intra_test_barrier",
		  (params->intra_test_barrier ? "true" : "false"));
	  fprintf(stdout, "\t%s = %d min\n", "run_time_limit",
		  params->run_time_limit);
	  fprintf(stdout, "\t%s = %s\n", "keep_file",
		  (params->keep_file ? "true" : "false"));
	  fprintf(stdout, "\t%s = %s\n", "use_existing_file",
		  (params->use_existing_file ? "true" : "false"));
	  fprintf(stdout, "\t%s = %s\n", "use_rep_in_file_name",
		  (params->use_rep_in_file_name ? "true" : "false"));
	  fprintf(stdout, "\t%s = %s\n", "dir_per_file",
		  (params->dir_per_file ? "true" : "false"));
	  fprintf(stdout, "\t%s = %s\n", "reorder_tasks",
		  (params->reorder_tasks ? "true" : "false"));
	  fprintf(stdout, "\t%s = %d\n", "reorder_tasks_offset",
		  params->reorder_tasks_offset);

	  fprintf(stdout, "\t%s = %s\n", "single_io_attempt",
		  (params->single_io_attempt ? "true" : "false"));
	}

      fprintf(stdout, "\n");

      fflush(stdout);
    }
} /* display_params (iore_params_t *) */

/*
 * Shows statistics about the file system.
 */
void
display_fs_info (char *file_name)
{
  struct statvfs stat;
  char *parent_dir;
  char resolved_path[MAXPATHLEN];
  long long int fs_size;
  long long int fs_free;
  long long int fs_used;
  double fs_util;
  long long int i_size;
  long long int i_free;
  long long int i_used;
  double i_util;

  if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
    {
      parent_dir = get_parent_path(file_name);

      if (statvfs(parent_dir, &stat) == 0)
	{
	  /* expand symbolic links and references in path */
	  if (realpath(parent_dir, resolved_path) == NULL)
	    {
	      WARN("Failed resolving parent dir path");
	      strcpy(resolved_path, parent_dir);
	    }
	  
	  /* blocks */
	  fs_size = stat.f_blocks * stat.f_frsize;
	  fs_free = stat.f_bfree * stat.f_frsize;
	  fs_used = fs_size - fs_free;
	  fs_util = ((double) fs_used / (double) fs_size) * 100;
	  
	  /* inodes */
	  i_size = stat.f_files;
	  i_free = stat.f_ffree;
	  i_used = i_size - i_free;
	  i_util = ((double) i_used / (double) i_size) * 100;
	  
	  fprintf(stdout, "Path: %s\n", resolved_path);
	  fprintf(stdout, "\t%12s\t%12s\t%12s\t%s\n", "Size", "Free", "Used",
		  "Used%");
	  fprintf(stdout, "%s\t%12s\t%12s\t%12s\t%.1f%%\n", "FS",
		  human_readable(fs_size, 0), human_readable(fs_free, 0),
		  human_readable(fs_used, 0), fs_util);
	  fprintf(stdout, "%s\t%12lli\t%12lli\t%12lli\t%.1f%%\n", "Inodes",
		  i_size, i_free, i_used, i_util);
	  fprintf(stdout, "\n");
	  fflush(stdout);
	}
      else
	{
	  WARN("Failed obtaining file system statistics");
	}
    }
} /* display_fs_info (char *) */

/*
 * Shows the header for results of repetitions of an experiment run.
 */
void
display_rep_header()
{
  if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
    {
      fprintf(stdout, "%s %10s  %12s  %10s  %12s  %12s %5s\n", "access",
	      "open(s)","io(s)", "close(s)", "total(s)", "tput(MiB/s)", "iter");
      fflush(stdout);
    }
} /* display_rep_header() */

/*
 * Shows the summary results of a single repetition of a read/write test.
 */
void
display_test_results (access_t access, int r)
{
  iore_time_t summary[R_CLOSE_STOP + 1] = { 0 };
  iore_size_t data_moved = 0;
  char *format = "%s %10.4f  %12.4f  %10.4f  %12.4f  %12.4f %5d\n";
  MPI_Op op;
  int i;

  if (task->verbosity >= NORMAL)
    {
      if (access == WRITE)
	{
	  /* find the minimum time for each even numbered timer (start timers),
	     and the maximum time for each odd numbered timer (stop timers) */
	  for (i = W_OPEN_START; i <= W_CLOSE_STOP; i++)
	    {
	      op = i % 2 == 0 ? MPI_MIN : MPI_MAX;
	      MPI_TRYCATCH(MPI_Reduce (&task->timer[i][r], &summary[i], 1,
				       MPI_DOUBLE, op, MASTER_RANK,
				       task->comm),
			   "Failed to summarize write test results");
	    }

	  /* compute the total amount of data moved in the test */
	  MPI_TRYCATCH(MPI_Reduce (&task->data_moved[WRITE][r], &data_moved,
				   1, MPI_LONG_LONG_INT, MPI_SUM, MASTER_RANK,
				   task->comm),
		       "Failed to summarize write test results");

	  if (task->rank == MASTER_RANK)
	    {
	      fprintf (stdout, format, "write",
		       summary[W_OPEN_STOP] - summary[W_OPEN_START],
		       summary[W_STOP] - summary[W_START],
		       summary[W_CLOSE_STOP] - summary[W_CLOSE_START],
		       summary[W_CLOSE_STOP] - summary[W_OPEN_START],
		       (((double)(data_moved / MEBIBYTE)) /
			(summary[W_CLOSE_STOP] - summary[W_OPEN_START])),
		       r);
	      fflush (stdout);
	    }
	}
      else /* READ */
	{
	  /* find the minimum time for each even numbered timer (start timers),
	     and the maximum time for each odd numbered timer (stop timers) */
	  for (i = R_OPEN_START; i <= R_CLOSE_STOP; i++)
	    {
	      op = i % 2 == 0 ? MPI_MIN : MPI_MAX;
	      MPI_TRYCATCH(MPI_Reduce (&task->timer[i][r], &summary[i], 1,
				       MPI_DOUBLE, op, MASTER_RANK,
				       task->comm),
			   "Failed to summarize read test results");
	    }

	  /* compute the total amount of data moved in the test */
	  MPI_TRYCATCH(MPI_Reduce (&task->data_moved[READ][r], &data_moved,
				   1, MPI_LONG_LONG_INT, MPI_SUM, MASTER_RANK,
				   task->comm),
		       "Failed to summarize read test results");

	  if (task->rank == MASTER_RANK)
	    {
	      fprintf (stdout, format, "read",
		       summary[R_OPEN_STOP] - summary[R_OPEN_START],
		       summary[R_STOP] - summary[R_START],
		       summary[R_CLOSE_STOP] - summary[R_CLOSE_START],
		       summary[R_CLOSE_STOP] - summary[R_OPEN_START],
		       (((double)(data_moved / MEBIBYTE)) /
			(summary[R_CLOSE_STOP] - summary[R_OPEN_START])),
		       r);
	      fflush (stdout);
	    }
	}
    }
} /* display_test_results (access_t, int) */

/*
 * Shows timing results for each operation and task.
 *
 * IMPORTANT: it is supposed to be used only at a DEBUG verbosity level due to 
 * the large amount of output that can be generated. This condition is not 
 * verified inside the function to avoid duplicated comparison since it becomes
 * more semantic to verify this in the execution flow, before calling the 
 * function.
 */
void
display_per_task_results (access_t access, int r)
{
  char access_desc[MAX_STR_LEN];
  char timer_desc[MAX_STR_LEN];
  int i, first, last;
  
  if (access == WRITE)
    {
      strcpy (access_desc, "write");
      first = W_OPEN_START;
      last = W_CLOSE_STOP;
    }
  else /* READ */
    {
      strcpy (access_desc, "read");
      first = R_OPEN_START;
      last = R_CLOSE_STOP;
    }

  for (i = first; i <= last; i++)
    {
      switch (i)
	{
	case W_OPEN_START:
	  strcpy (timer_desc, "write open start");
	  break;
	case W_OPEN_STOP:
	  strcpy (timer_desc, "write open stop");
	  break;
	case W_START:
	  strcpy (timer_desc, "write start");
	  break;
	case W_STOP:
	  strcpy (timer_desc, "write stop");
	  break;
	case W_CLOSE_START:
	  strcpy (timer_desc, "write close start");
	  break;
	case W_CLOSE_STOP:
	  strcpy (timer_desc, "write close stop");
	  break;
	case R_OPEN_START:
	  strcpy (timer_desc, "read open start");
	  break;
	case R_OPEN_STOP:
	  strcpy (timer_desc, "read open stop");
	  break;
	case R_START:
	  strcpy (timer_desc, "read start");
	  break;
	case R_STOP:
	  strcpy (timer_desc, "read stop");
	  break;
	case R_CLOSE_START:
	  strcpy (timer_desc, "read close start");
	  break;
	case R_CLOSE_STOP:
	  strcpy (timer_desc, "read close stop");
	  break;
	}

      fprintf (stdout, "Run %d: Iter=%d, Task=%d, Time=%f, %s\n",
	       task->run_id, r, task->rank, task->timer[i][r], timer_desc);
      fflush (stdout);
    }
} /* display_per_task_results (access_t, int) */
