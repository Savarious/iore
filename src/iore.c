#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "iore.h"
#include "util.h"

/*****************************************************************************
 * V A R I A B L E S                                                         *
 *****************************************************************************/

static int error_count = 0;

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Displays command line options and help.
 */
static void
usage(char **argv)
{
  char *opts[] = {
    "OPTIONS:",
    " -h    display command line options and help",
    " ",
    "* NOTE: S is a string, N is an integer number.",
    " ",
    ""
  };
  
  fprintf(stdout, "Usage: %s [OPTIONS]\n\n", *argv);
  int i;
  for (i = 0; strlen(opts[i]) > 0; i++) {
    fprintf(stdout, "%s\n", opts[i]);
  }
  
  return;
}


/*****************************************************************************
 * M A I N                                                                   *
 *****************************************************************************/

int
main(int argc, char **argv)
{
  int nprocs;
  int rank;
  IORE_test_t *tests_head;
  IORE_test_t *tests_current;
  
  /* check for the -h or --help options (display usage) in the command
     line before starting MPI. */
  int i;
  for (i = 1; i < argc; i++) {
    if (STREQUAL(argv[i], "-h") || STREQUAL(argv[i], "--help")) {
      usage(argv);
      exit(EXIT_SUCCESS);
    }
  }

  /* start MPI */
  IORE_MPI_CHECK(MPI_Init(&argc, &argv), "Cannot initialize MPI");
  IORE_MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nprocs),
		 "Cannot get the number of MPI ranks");
  IORE_MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank), "Cannot get rank");

  /* TODO: continue... */
  fprintf(stdout, "Started MPI...\n");

  /* finalize MPI */
  IORE_MPI_CHECK(MPI_Finalize(), "Cannot finalize MPI");

  return(error_count);
}
