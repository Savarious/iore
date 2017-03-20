#include <time.h>

#include "util.h"

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Get current time as a string.
 */
char *
get_time_string ()
{
  static time_t curtime;
  char *curtimestr;

  curtime = time (NULL);
  if (curtime == -1)
    {
      FATAL("Failed to get current time.");
    }
  else
    {
      curtimestr = ctime (&curtime);
      if (curtimestr == NULL)
	FATAL("Failed to convert current time to string.");
    }

  return curtimestr;
} /* get_time_string () */
