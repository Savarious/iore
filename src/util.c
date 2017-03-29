#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
  char *curtimestr;

  curtime = time (NULL);
  if (curtime == -1)
      FATAL("Failed to get current time.");
  else
    {
      curtimestr = ctime(&curtime);
      if (curtimestr == NULL)
	FATAL("Failed to convert current time to string.");
    }

  return curtimestr;
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
