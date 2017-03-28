#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "parse_file.h"
#include "iore_experiment.h"
#include "iore_run.h"
#include "iore_params.h"
#include "util.h"

#include "json.h"

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

static char *get_file_data (char *);
static void parse_file_run (json_value *, iore_run_t *);
static void parse_file_params (json_value *, iore_params_t *);
static iore_size_t string_to_bytes (char *);

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

/*
 * Parse file and define an experiment.
 */
void
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
  json_value *value;
  char *param_name;
  int num_params;
  int num_errors = 0;
  char *errmsg_acc = NULL;
  int length;
  int i, j;
  
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
	  if (param->type != json_integer || param->u.integer < 0)
	    {
	      strcat(errmsg_acc, "num_tasks must be a positive integer\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->num_tasks = param->u.integer;
	    }
	}
      else if (STREQUAL(param_name, "api"))
	{
	  if (param->type != json_string)
	    {
	      strcat(errmsg_acc, "api must be a string\n");
	      num_errors++;
	    }
	  else
	    {
	      strcpy(iore_params->api, param->u.string.ptr);
	    }
	}
      else if (STREQUAL(param_name, "sharing_policy"))
	{
	  if (param->type != json_string ||
	      !(STREQUAL(param->u.string.ptr, "SHARED_FILE") ||
		STREQUAL(param->u.string.ptr, "FILE_PER_PROCESS")))
	    {
	      strcat(errmsg_acc,
		     "sharing_policy must be either \"SHARED_FILE\""
		     "or \"FILE_PER_PROCESS\"\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->sharing_policy = STREQUAL(param->u.string.ptr,
						     "SHARED_FILE") ?
		SHARED_FILE : FILE_PER_PROCESS;
	    }
	}
      else if (STREQUAL(param_name, "access_pattern"))
	{
	  if (param->type != json_string ||
	      !(STREQUAL(param->u.string.ptr, "SEQUENTIAL") ||
		STREQUAL(param->u.string.ptr, "RANDOM")))
	    {
	      strcat(errmsg_acc,
		     "access_pattern must be either \"SEQUENTIAL\""
		     "or \"RANDOM\"\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->access_pattern = STREQUAL(param->u.string.ptr,
						     "SEQUENTIAL") ?
		SEQUENTIAL : RANDOM;
	    }
	}
      else if (STREQUAL(param_name, "block_sizes"))
	{
	  if (param->type == json_integer)
	    {
	      if (param->u.integer < 1)
		{
		  strcat(errmsg_acc, "block_sizes must be greater than 0\n");
		  num_errors++;
		}
	      else
		{
		  iore_params->block_sizes =
		    (iore_size_t *) malloc(sizeof(iore_size_t));
		  iore_params->block_sizes[0] = param->u.integer;
		}
	    }
	  else if (param->type == json_string)
	    {
	      iore_params->block_sizes =
		(iore_size_t *) malloc(sizeof(iore_size_t));

	      if ((iore_params->block_sizes[0] =
		   string_to_bytes(param->u.string.ptr)) < 1)
		{
		  strcat(errmsg_acc, "block_sizes must be greater than 0\n");
		  num_errors++;
		}
	    }
	  else if (param->type == json_array)
	    {
	      length = param->u.array.length;
	      iore_params->block_sizes =
		(iore_size_t *) malloc(length * sizeof(iore_size_t));

	      for (j = 0; j < length; j++)
		{
		  value = param->u.array.values[j];
		  if (value->type == json_integer)
		    {
		      if (value->u.integer < 1)
			{
			  strcat(errmsg_acc,
				 "block_sizes must be greater than 0\n");
			  num_errors++;
			}
		      else
			{
			  iore_params->block_sizes[j] = value->u.integer;
			}
		    }
		  else if (value->type == json_string)
		    {
		      if ((iore_params->block_sizes[j] =
			   string_to_bytes(value->u.string.ptr)) < 1)
			{
			  strcat(errmsg_acc,
				 "block_sizes must be greater than 0\n");
			  num_errors++;
			}
		    }
		  else
		    {
		      strcat(errmsg_acc, "block_sizes must be "
			     "by an integer number of bytes, "
			     "or a string formed by an integer plus a unit\n");
		      num_errors++;
		    }
		}
	    }
	  else
	    {
	      strcat(errmsg_acc, "block_sizes must be "
		     "an integer number of bytes, "
		     "a string formed by an integer plus a unit, "
		     "or an array of integers or of integers plus units\n");
	      num_errors++;
	    }
	}
      else if (STREQUAL(param_name, "transfer_sizes"))
	{
	  if (param->type == json_integer)
	    {
	      if (param->u.integer < 1)
		{
		  strcat(errmsg_acc, "transfer_sizes must be greater than 0\n");
		  num_errors++;
		}
	      else
		{
		  iore_params->transfer_sizes =
		    (iore_size_t *) malloc(sizeof(iore_size_t));
		  iore_params->transfer_sizes[0] = param->u.integer;
		}
	    }
	  else if (param->type == json_string)
	    {
	      iore_params->transfer_sizes =
		(iore_size_t *) malloc(sizeof(iore_size_t));

	      if ((iore_params->transfer_sizes[0] =
		   string_to_bytes(param->u.string.ptr)) < 1)
		{
		  strcat(errmsg_acc, "transfer_sizes must be greater than 0\n");
		  num_errors++;
		}
	    }
	  else if (param->type == json_array)
	    {
	      length = param->u.array.length;
	      iore_params->transfer_sizes =
		(iore_size_t *) malloc(length * sizeof(iore_size_t));

	      for (j = 0; j < length; j++)
		{
		  value = param->u.array.values[j];
		  if (value->type == json_integer)
		    {
		      if (value->u.integer < 1)
			{
			  strcat(errmsg_acc,
				 "transfer_sizes must be greater than 0\n");
			  num_errors++;
			}
		      else
			{
			  iore_params->transfer_sizes[j] = value->u.integer;
			}
		    }
		  else if (value->type == json_string)
		    {
		      if ((iore_params->transfer_sizes[j] =
			   string_to_bytes(value->u.string.ptr)) < 1)
			{
			  strcat(errmsg_acc,
				 "transfer_sizes must be greater than 0\n");
			  num_errors++;
			}
		    }
		  else
		    {
		      strcat(errmsg_acc, "transfer_sizes must be "
			     "by an integer number of bytes, "
			     "or a string formed by an integer plus a unit\n");
		      num_errors++;
		    }
		}
	    }
	  else
	    {
	      strcat(errmsg_acc, "transfer_sizes must be "
		     "an integer number of bytes, "
		     "a string formed by an integer plus a unit, "
		     "or an array of integers or of integers plus units\n");
	      num_errors++;
	    }
	}
      else if (STREQUAL(param_name, "write_test"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc, "write_test must be either true or false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->write_test = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "read_test"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc, "read_test must be either true or false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->read_test = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "ref_num"))
	{
	  if (param->type != json_integer)
	    {
	      strcat(errmsg_acc, "ref_num must be an integer\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->ref_num = param->u.integer;
	    }
	}
      else if (STREQUAL(param_name, "root_file_name"))
	{
	  if (param->type != json_string)
	    {
	      strcat(errmsg_acc, "root_file_name must be a string\n");
	      num_errors++;
	    }
	  else
	    {
	      strcpy(iore_params->root_file_name, param->u.string.ptr);
	    }
	}
      else if (STREQUAL(param_name, "num_repetitions"))
	{
	  if (param->type != json_integer || param->u.integer < 1)
	    {
	      strcat(errmsg_acc,
		     "num_repetitions must be a greater than zero\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->num_repetitions = param->u.integer;
	    }
	}
      else if (STREQUAL(param_name, "inter_test_delay"))
	{
	  if (param->type != json_integer || param->u.integer < 0)
	    {
	      strcat(errmsg_acc,
		     "inter_test_delay must be a positive integer\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->inter_test_delay = param->u.integer;
	    }
	}
      else if (STREQUAL(param_name, "intra_test_barrier"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc,
		     "intra_test_barrier must either true or false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->intra_test_barrier = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "run_time_limit"))
	{
	  if (param->type != json_integer || param->u.integer < 0)
	    {
	      strcat(errmsg_acc, "run_time_limit must be a positive integer\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->run_time_limit = param->u.integer;
	    }
	}
      else if (STREQUAL(param_name, "keep_file"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc, "keep_file must be either true of false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->keep_file = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "use_existing_file"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc,
		     "use_exiting_file must be either true of false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->use_existing_file = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "use_rep_in_file_name"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc,
		     "use_rep_in_file_name must be either true of false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->use_rep_in_file_name = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "dir_per_file"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc, "dir_per_file must be either true of false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->dir_per_file = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "reorder_tasks"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc,
		     "reorder_tasks must be either true of false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->reorder_tasks = param->u.boolean;
	    }
	}
      else if (STREQUAL(param_name, "reorder_tasks_offset"))
	{
	  if (param->type != json_integer || param->u.integer < 0)
	    {
	      strcat(errmsg_acc,
		     "reorder_tasks_offset must be a positive integer\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->reorder_tasks_offset = param->u.integer;
	    }
	}
      else if (STREQUAL(param_name, "verbosity"))
	{
	  if (param->type != json_string)
	    {
	      strcat(errmsg_acc, "verbosity must be one of the following: "
		     "\"QUIET\", \"NORMAL\", \"VERBOSE\", \"VERY_VERBOSE\", "
		     "\"DEBUG\"\n");
	      num_errors++;
	    }
	  else
	    {
	      if (STREQUAL(param->u.string.ptr, "QUIET"))
		iore_params->verbosity = QUIET;
	      else if (STREQUAL(param->u.string.ptr, "NORMAL"))
		iore_params->verbosity = NORMAL;
	      else if (STREQUAL(param->u.string.ptr, "VERBOSE"))
		iore_params->verbosity = VERBOSE;
	      else if (STREQUAL(param->u.string.ptr, "VERY_VERBOSE"))
		iore_params->verbosity = VERY_VERBOSE;
	      else if (STREQUAL(param->u.string.ptr, "DEBUG"))
		iore_params->verbosity = DEBUG;
	      else
		{
		  strcat(errmsg_acc, "verbosity must be one of the following: "
			 "\"QUIET\", \"NORMAL\", \"VERBOSE\", \"VERY_VERBOSE\","
			 " \"DEBUG\"\n");
		  num_errors++;
		}
	    }
	}
      else if (STREQUAL(param_name, "single_io_attempt"))
	{
	  if (param->type != json_boolean)
	    {
	      strcat(errmsg_acc,
		     "single_io_attempt must be either true or false\n");
	      num_errors++;
	    }
	  else
	    {
	      iore_params->single_io_attempt = param->u.boolean;
	    }
	}
    } /* end of loop over parameters */
} /* parse_file_params (json_value *, iore_params_t *) */

/*
 * Converts a string of the form 2, 4k, 8K, 16m, 32m, 64g, 128G into bytes.
 * Considers base two units.
 */
static iore_size_t
string_to_bytes (char *size_str)
{
  iore_size_t size = 0;
  char unit;
  int n;

  n = sscanf(size_str, "%lld%c", &size, &unit);
  if (n == 2)
    {
      switch ((int) unit)
	{
	case 'k':
	case 'K':
	  size <<= 10;
	  break;
	case 'm':
	case 'M':
	  size <<= 20;
	  break;
	case 'g':
	case 'G':
	  size <<= 30;
	  break;
	default:
	  size = -1;
	}
    }
  else if (n == 0)
    {
      size = -1;
    }

  return (size);
} /* string_to_bytes (char *) */
