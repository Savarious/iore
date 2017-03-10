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
static void display_header(int argc, char **argv, enum VERBOSE verbose);
static void display_test_info(IORE_param_t *params);
static void display_summary(IORE_test_t *tests);
static void display_footer();
static IORE_test_t *setup_tests(int argc, char **argv);
static void run(IORE_test_t *test);
static void finalize(IORE_test_t *tests);

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
  IORE_test_t *tests;
  
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
    FATAL("No I/O backends compiled for IORE.");
  }

  IORE_MPI_CHECK(MPI_Init(&argc, &argv), "Cannot initialize MPI");
  IORE_MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nprocs),
		 "Cannot get the number of MPI ranks");
  IORE_MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank), "Cannot get rank");

  /* setup tests based on command line arguments */
  tests = setup_tests(argc, argv);

  display_header(argc, argv, tests->params.verbose);

  /* perform each test */
  IORE_test_t *test;
  for (test = tests; test != NULL; test = test->next) {
    if (rank == 0) {
      display_test_info(&test->params);
    }

    run(test);
  }

  display_summary(tests);

  display_footer();

  finalize(tests);

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
 * Displays the initial header (e.g., command line used, machine name, etc.).
 *
 * argc: number of arguments
 * argv: array of arguments
 * verbose: verbosity level
 */
static void display_header(int argc, char **argv, enum VERBOSE verbose)
{
  /* TODO: implement */
}

/*
 * Displays the test information.
 *
 * params: test parameters
 */
static void display_test_info(IORE_param_t *params)
{
  /* TODO: implement */
}

/*
 * Displays a summary of all tests.
 *
 * tests: tests list
 */
static void display_summary(IORE_test_t *tests)
{
  /* TODO: implement */
}

/*
 * Displays concluding information.
 */
static void display_footer()
{
  /* TODO: implement */
}

/*
 * Setup tests based on command line arguments.
 *
 * argc: number of arguments
 * argv: array of arguments
 */
static IORE_test_t *setup_tests(int argc, char **argv)
{
  /* TODO: consider possibility of specifying multiple tests */
  IORE_test_t *tests;

  tests = parse_cmd_line(argc, argv);

  /* TODO: implement */

  return(tests);
}

/*
 * Execute iterations of a single test.
 */
static void run(IORE_test_t *test)
{
  /* TODO: implement */
}

/*
 * Execute finalizing actions (e.g., destroy tests, etc.).
 */
static void finalize(IORE_test_t *test)
{
  /* TODO: implement */
}
