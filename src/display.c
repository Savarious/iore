#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h> /* uname */

#include "display.h"
#include "iore_task.h"
#include "util.h"
#include "iore.h"

/*****************************************************************************
 * G L O B A L S
 *****************************************************************************/

/*extern iore_task_t *task;*/

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
  display_version();
} /* display_splash() */

void
display_expt_header(char **argv)
{
  struct utsname buf;
  
  if (task->rank == MASTER_RANK)
    {
      fprintf(stdout, "Start: %s", current_time_str());
      fprintf(stdout, "Command line used: %s\n", *argv);

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

      /* TODO: display time deviation */
      
      fflush(stdout);
    }
} /* display_expt_header(char **) */
