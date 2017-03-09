#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "iore.h"
#include "util.h"
#include "ioreaio.h"
#include "parseopt.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

static void usage(char **argv);
static void display_splash();
static void display_header(int argc, char **argv, int verbose);
static IORE_test_t setup_tests(int argc, char **argv);

/*****************************************************************************
 * V A R I A B L E S                                                         *
 *****************************************************************************/

static int nprocs;
static int rank;
static int error_count = 0;

/* TODO: use #ifdef to check compiled interfaces */
static IORE_aio_t *available_ioreaio[] = {
  &ioreaio_posix,
  NULL
};

/*****************************************************************************
 * M A I N                                                                   *
 *****************************************************************************/

int main(int argc, char **argv)
{
  IORE_test_t tests;
  
  /* check for the -h or --help options (display usage) in the command
     line before starting MPI. */
  int i;
  for (i = 1; i < argc; i++) {
    if (STREQUAL(argv[i], "-h") || STREQUAL(argv[i], "--help")) {
      usage(argv);
      exit(EXIT_SUCCESS);
    }
  }

  display_splash();

  /* check for compiled I/O backend */
  if (available_ioreaio[0] == NULL) {
    ERR("No I/O backends compiled for IORE.");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  /* start MPI */
  IORE_MPI_CHECK(MPI_Init(&argc, &argv), "Cannot initialize MPI");
  IORE_MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nprocs),
		 "Cannot get the number of MPI ranks");
  IORE_MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank), "Cannot get rank");

  /* setup tests based on command line arguments */
  tests = setup_tests(argc, argv);

  display_header(argc, argv, tests->params->verbose);

  /* TODO: continue... */

  /* finalize MPI */
  IORE_MPI_CHECK(MPI_Finalize(), "Cannot finalize MPI");

  return(error_count);
}

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Displays command line options and help.
 * 
 * **argv: command line arguments
 */
static void usage(char **argv)
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

/*
 * Displays the splash screen.
 */
static void display_splash()
{
  /* TODO: include META_VERSION definition */
  fprintf(stdout, "IORE %s - The IOR-Extended Benchmark\n\n", "TBA");
  fflush(stdout);
}

/*
 * Displays the output header.
 *
 * argc: number of arguments
 * argv: array of arguments
 * verbose: verbosity level
 */
static void display_header(int argc, char **argv, int verbose)
{
  /* TODO: implement */
}

/*
 * Setup tests based on command line arguments.
 *
 * argc: number of arguments
 * argv: array of arguments
 */
static IORE_test_t setup_tests(int argc, char **argv)
{
  /* TODO: consider possibility of specifying multiple tests */
  IORE_test_t tests;

  tests = parse_cmd_line(argc, argv);

  /* TODO: implement */

  return(tests_head);
}
