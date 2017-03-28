#include <stdio.h>
#include <stdlib.h>

#include "iore_experiment.h"
#include "iore_run.h"
#include "iore_params.h"
#include "display.h"
#include "util.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       
 *****************************************************************************/

static int handle_preemptive_args (int, char **);

/*****************************************************************************
 * M A I N                                                                   
 *****************************************************************************/

int
main (int argc, char **argv)
{
  if (!handle_preemptive_args(argc, argv))
    {
      display_splash();
    }

  exit (EXIT_SUCCESS);
} /* main (int, char **) */

/*****************************************************************************
 * F U N C T I O N S                                                         *
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
