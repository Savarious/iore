#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "iore_experiment.h"
#include "iore_run.h"
#include "iore_params.h"
#include "util.h"

#include "json.h"

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

static void parse_opts (int, char **, iore_experiment_t *);
static void parse_file (char *, iore_experiment_t *);
static char *get_file_data (char *);
static void parse_file_run (json_value *, iore_run_t *);
static void parse_file_params (json_value *, iore_params_t *);

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
  experiment->front = run;
  experiment->rear = run;
  experiment->size = 1;

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
  iore_run_t *iore_run = NULL;
  char *file_data;
  json_char *json;
  json_value *root;
  json_value *runs;
  int num_runs;
  int i;

  file_data = get_file_data(file_name);

  json = (json_char *) file_data;
  root = json_parse(json, strlen(file_data));
  /* TODO: is it possible to free file_data at this point? */
  if (root == NULL)
    {
      ERRF("Unable to parse experiment definition data in file %s.", file_name);
      free(file_data);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  if (root->type != json_object)
    {
      ERR("Experiment definition file is wrongly formatted.");
      free(file_data);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  /* TODO: check object names? */
  
  runs = root->u.object.values[0].value;
  if (runs == NULL || runs->type != json_array)
    {
      ERR("Experiment definition file is wrongly formatted.");
      free(file_data);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  num_runs = runs->u.array.length;
  if (num_runs == 0)
    {
      ERR("No run defined for the experiment; aborting.");
      free(file_data);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
  /* loop over runs */
  for (i = 0; i < num_runs; i++)
    {
      if (iore_run == NULL)
	{ /* if the first run, change the default parameters */
	  iore_run = experiment->front;
	}
      else
	{ /* create a new run and add to the linked list */
	  iore_run->next = new_run(i, new_params());
	  iore_run = iore_run->next;
	}
      
      parse_file_run(runs->u.array.values[i], iore_run);      
    } /* end of loop over runs */

  experiment->rear = iore_run;
  
  json_value_free(root);
  free(file_data);

  return;
} /* parse_file (char *, iore_experiment_t *) */

/*
 * Open a file, read its content into a string, and close the file.
 */
static char *
get_file_data (char *file_name)
{
  char *data;
  FILE *fp;
  int length;

  fp = fopen(file_name, "rt");
  if (fp == NULL)
    {
      FATALF("Failed to open file %s.", file_name);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  if (length == -1)
    {
      FATALF("Failed to get the size of file %s.", file_name);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
  else if (length == 0)
    {
      ERRF("No experiment definition found on file %s; aborting.", file_name);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  data = (char *) malloc(length + 1); /* +1 to include string termination \0 */
  if (data == NULL)
    {
      FATAL("Failed to allocated memory for file contents.");
      free(data);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
  
  if (fread(data, length, 1, fp) != 1)
    {
      FATALF("Failed to read file %s contents.", file_name);
      free(data);
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
  fclose(fp);

  data[length] = '\0';

  return (data);
} /* get_file_data (char *) */

/*
 * Parse a JSON run object into a experiment run.
 */
static void
parse_file_run (json_value *run, iore_run_t *iore_run)
{
  if (run == NULL || run->type != json_object)
    {
      ERR("Experiment definition file is wrongly formatted.");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

  parse_file_params(run->u.object.values[0].value, &iore_run->params);
   
} /* parse_file_run (json_value *, iore_run_t *) */

/*
 * Parse a JSON params object into a run parameters.
 */
static void
parse_file_params (json_value *params, iore_params_t *iore_params)
{
  json_value *param;
  char *param_name;
  int num_params;
  int num_errors = 0;
  int i;
  
  /* no need to check for params == NULL; if so, run with default */
  if (params->type != json_object)
    {
      ERR("Experiment definition file is wrongly formatted.");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
  
  num_params = params->u.object.length;
  /* loop over parameters */
  for (i = 0; i < num_params; i++)
    {
      param = params->u.object.values[i].value;
      param_name = params->u.object.values[i].name;

      if (STREQUAL(param_name, "num_tasks"))
	{
	  if (param->type != json_integer)
	    {
	      num_errors++;
	      /* TODO: create an error message, but do not display it now */
	    }
	}
	  
    } /* end of loop over parameters */ 
} /* parse_file_params (json_value *, iore_params_t *) */
