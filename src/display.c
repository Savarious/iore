#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* environ */
#include <sys/utsname.h> /* uname */

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
  char *dir;
  
  if (task->verbosity >= NORMAL && task->rank == MASTER_RANK)
    {
      fprintf(stdout, "Test %d started: %s\n", id, current_time_str());

      if (task->verbosity >= VERY_VERBOSE)
	{
	  dir = get_parent_path(params->root_file_name);
	  /* TODO: display file system free space */
	}

      display_params(params);

      fprintf(stdout, "Participating tasks: %d\n\n", params->num_tasks);
      
      fflush(stdout);
    }
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
	      human_readable(params->block_sizes[0]));
      for (i = 1; i < params->block_sizes_length; i++)
	fprintf(stdout, ", %s", human_readable(params->block_sizes[i]));
      fprintf(stdout, " }\n");

      fprintf(stdout, "\t%s = { %s", "transfer_sizes",
	      human_readable(params->transfer_sizes[0]));
      for (i = 1; i < params->transfer_sizes_length; i++)
	fprintf(stdout, ", %s", human_readable(params->transfer_sizes[i]));
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
	  fprintf(stdout, "\t%s = %d s\n", "inter_test_delay",
		  params->inter_test_delay);
	  fprintf(stdout, "\t%s = %s\n", "intra_test_barrier",
		  (params->intra_test_barrier ? "true" : "false"));
	  fprintf(stdout, "\t%s = %d m\n", "run_time_limit",
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
