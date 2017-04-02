#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h> /* MAXPATHLEN */
#include <string.h>
#include <mpi.h>

#include "util.h"
#include "iore_task.h"

/*****************************************************************************
 * G L O B A L S
 *****************************************************************************/

extern iore_task_t *task;

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

/*
 * Returns the current time as a string.
 */
char *
current_time_str()
{
  static time_t curtime;
  char *curtimestr = NULL;

  curtime = time (NULL);
  if (curtime == -1)
      FATAL("Failed to get current time.");
  else
    {
      curtimestr = ctime(&curtime);
      if (curtimestr == NULL)
	FATAL("Failed to convert current time to string.");
    }

  return (curtimestr);
} /* current_time_str() */

/*
 * Returns a floating-point number of seconds, representing elapsed time.
 */
iore_time_t
current_time()
{
  iore_time_t wtime = 0;
  
  wtime = MPI_Wtime();
  
  if (wtime < 0)
    FATAL("Failed to get timestamp using MPI_Wtime().");

  wtime -= task->wclock_delta;

  return (wtime);
} /* current_time() */

/*
 * Synchronize random number generation across all tasks in an MPI communicator
 * through broadcasting a seed from the master rank.
 */
void
sync_rand_gen(MPI_Comm comm)
{
  unsigned int seed;
  struct timeval time;

  if (task->rank == 0)
    {
      gettimeofday(&time, (struct timezone *) NULL);
      seed = time.tv_usec;
    }

  MPI_TRYCATCH(MPI_Bcast(&seed, 1, MPI_INT, MASTER_RANK, comm),
	       "Failed to broadcast the seed for the PRNG");

  srandom(seed);
} /* sync_rand_gen(MPI_Comm) */

/*
 * Get a size in bytes in a more human readable unit.
 * Considers base two units: KiB, MiB, GiB.
 */
char *
human_readable(iore_size_t size, int precision)
{
  char *size_str = (char *) malloc (MAX_STR_LEN * sizeof(char));
  
  if (size >= TEBIBYTE)
    {
      if ((size % (iore_size_t) TEBIBYTE) == 0)
	sprintf(size_str, "%lli TiB", (size / TEBIBYTE));
      else
	sprintf(size_str, "%.*f TiB", precision, ((double) size / TEBIBYTE));
    }
  else if (size >= GIBIBYTE)
    {
      if ((size % (iore_size_t) GIBIBYTE) == 0)
	sprintf(size_str, "%lli GiB", (size / GIBIBYTE));
      else
	sprintf(size_str, "%.*f GiB", precision, ((double) size / GIBIBYTE));
    }
  else if (size >= MEBIBYTE)
    {
      if ((size % (iore_size_t) MEBIBYTE) == 0)
	sprintf(size_str, "%lli MiB", (size / MEBIBYTE));
      else
	sprintf(size_str, "%.*f MiB", precision, ((double) size / MEBIBYTE));
    }
  else if (size >= KIBIBYTE)
    {
      if ((size % (iore_size_t) KIBIBYTE) == 0)
	sprintf(size_str, "%lli KiB", (size / KIBIBYTE));
      else
	sprintf(size_str, "%.*f KiB", precision, ((double) size / KIBIBYTE));
    }
  else if (size >= 1)
    sprintf(size_str, "%f B", ((double) size));
  else
    sprintf(size_str, "-");

  return (size_str);
} /* human_readable(iore_size_t, int) */

/*
 * Returns the path of the parent directory of the file/directory addressed by
 * the path argument.
 */
char *
get_parent_path(char *path)
{
  int i;
  char *parent;
  int found = FALSE;

  parent = (char *) malloc(MAXPATHLEN * sizeof(char));

  /* ignores the last char to cope with path to directory ending with / */
  i = strlen(path) - 1;
  while (i > 0 && !found)
    {
      if (path[i] == '/')
	{
	  strncpy(parent, path, i);
	  found = TRUE;
	}

      i--;
    }

  if (!found)
    {
      parent[0] = '.';
      parent[1] = '/';
      parent[2] = '\0';
    }
  else
    parent[i+1] = '\0';

  return (parent);
} /* get_parent_path(char *) */

/*
 * Returns only the file name from the full path.
 */
char *
get_file_name (char *path)
{
  int i;
  char *file;
  int found = FALSE;

  file = (char *) malloc (MAXPATHLEN * sizeof(char));

  /* ignores the last char to cope with path to directory ending with / */
  i = strlen(path) - 1;
  while (i > 0 && !found)
    {
      if (path[i] == '/' || path[i] == '.')
	{
	  file = path + (i + 1);
	  found = TRUE;
	}

      i--;
    }

  if (!found)
    file = path;
  else
    file[strlen(file)] = '\0';

  return (file);
} /* get_file_name (char *) */
