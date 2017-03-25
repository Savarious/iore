#include <mpi.h>

#include "iore_experiment.h"
#include "iore_run.h"
#include "iore_params.h"
#include "util.h"

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

static void parse_opts (int, char **, iore_experiment_t *);
static void parse_file (char *, iore_experiment_t *);

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

/*
 * Create an experiment from command-line arguments.
 */
iore_experiment_t *
new_experiment (int argc, char **argv)
{
  iore_experiment_t *experiment = NULL;
  iore_run_t *run = NULL;
  iore_params_t *params = NULL;

  /* initialize a default experiment */
  params = new_params();
  run = new_run(0, params);
  experiment = (iore_experiment_t *) malloc(sizeof(iore_experiment_t));
  if (experiment == NULL)
    {
      FATAL("Failed to allocate memory for experiment structure");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  parse_opts(argc, argv, experiment);
  
  return (experiment);
} /* *new_experiment (int, char **) */


/*
 * Parse command line arguments and define an experiment
 */
static void
parse_opts (int argc, char **argv, iore_experiment_t *experiment)
{
  int i;
  char *expt_file_name = NULL;
  
  /* 
   * File defined experiment option (-f) takes precedence over command line
   * arguments. In other words, if the -f option is used, all other arguments
   * are ignored. 
   */
  i = 0;
  while (i < argc - 1 && expt_file_name == NULL)
    {
      if (STREQUAL(argv[i], "-f"))
	expt_file_name = argv[i+1];
      i++;
    }
  
  if (expt_file_name != NULL)
    {
      parse_file(expt_file_name, experiment);
    }
  else
    {
      /* TODO: parse command line args */
    }

  return;
} /* parse_opts (int, char **, iore_experiment_t *) */

/*
 * Parse file and define an experiment.
 */
static void
parse_file (char *file_name, iore_experiment_t *experiment)
{
  /* TODO: parse JSON file */
} /* parse_file (char *, iore_experiment_t *) */
