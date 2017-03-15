#ifndef _UTIL_H
#define _UTIL_H

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*****************************************************************************
 * D E F I N I T I O N S                                                     *
 *****************************************************************************/

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef MASTER_RANK
#define MASTER_RANK 0
#endif

#define MAX_STR 1024 /* max string length */
#define MAX_RETRY 10000 /* max number of retries for POSIX I/O */

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

char *get_time_string();

/*****************************************************************************
 * D E C L A R A T I O N S                                                   *
 *****************************************************************************/

enum VERBOSE {
  VERBOSE_0 = 0,
  VERBOSE_1 = 1,
  VERBOSE_2 = 2,
  VERBOSE_3 = 3,
  VERBOSE_4 = 4,
  VERBOSE_5 = 5
};

enum ACCESS {
  WRITE,
  WRITECHECK,
  READ,
  READCHECK
};

enum SHARING_POLICY {
  SHARED_FILE,
  FILE_PER_PROCESS
};

/*****************************************************************************
 * M A C R O S                                                               *
 *****************************************************************************/

/*
 * Compares two strings for equality.
 *
 * A: First string
 * B: Second string
 */
#define STREQUAL(A, B) strcmp(A, B) == 0

/*
 * Displays a custom error message, the MPI error string for a given
 * MPI error, and then exit the program.
 *
 * ERROR_CODE: error code from MPI
 * IORE_MSG: custom error message
 */
#define IORE_MPI_CHECK(ERROR_CODE, IORE_MSG) do {			\
    if (ERROR_CODE != MPI_SUCCESS) {					\
      char error_string[MPI_MAX_ERROR_STRING];				\
      int error_string_length;						\
      									\
      MPI_Error_string(ERROR_CODE, error_string, &error_string_length);	\
      fprintf(stderr, "IORE ERROR: %s, MPI %s, (%s:%d)\n", IORE_MSG,	\
	      error_string, __FILE__, __LINE__);			\
      fflush(stderr);							\
      MPI_Abort(MPI_COMM_WORLD, -1);					\
    }									\
  } while(0)
  
/*
 * Displays a fatal error message and exit the program.
 *
 * IORE_MSG: custom message
 */
#define FATAL(IORE_MSG) do {						\
    fprintf(stderr, "IORE FATAL: %s, errno %d, %s (%s:%d).\n",		\
	    IORE_MSG, errno, strerror(errno), __FILE__, __LINE__);	\
    fflush(stderr);							\
    MPI_Abort(MPI_COMM_WORLD, -1);					\
  } while(0)

/*
 * Displays an error message.
 *
 * IORE_MSG: custom message
 * VERBOSE: verbosity level
 */
#define ERR(IORE_MSG, VERBOSE) do {					\
    if (VERBOSE <= VERBOSE_2) {						\
      fprintf(stderr, "IORE ERROR: %s, errno %d, %s.\n", IORE_MSG,	\
	      errno, strerror(errno));					\
    } else {								\
      fprintf(stderr, "IORE ERROR: %s, errno %d, %s (%s:%d).\n",	\
	      IORE_MSG, errno, strerror(errno), __FILE__, __LINE__);	\
    }									\
  } while(0)

/*
 * Displays a warning message.
 * 
 * IORE_MSG: custom message
 * VERBOSE: verbosity level
 */
#define WARN(IORE_MSG, VERBOSE) do {					\
    if (VERBOSE <= VERBOSE_2) {						\
      fprintf(stdout, "IORE WARNING: %s.\n", IORE_MSG);			\
    } else {								\
      fprintf(stdout, "IORE WARNING: %s, (%s:%d).\n", IORE_MSG,		\
	      __FILE__, __LINE__);					\
    }									\
    fflush(stdout);							\
  } while(0)

/*
 * Displays a formatted warning message.
 * 
 * IORE_MSG_FORMAT: string format for printf
 * VERBOSE: verbosity level
 */
#define WARNF(IORE_MSG_FMT, VERBOSE, ...) do {	\
    char msg[MAX_STR];				\
    sprintf(msg, IORE_MSG_FMT, __VA_ARGS__);	\
    WARN(msg, VERBOSE);				\
  } while(0)

/*
 * Displays a general message.
 * 
 * IORE_MSG_FMT: string format for printf
 */
#define INFOF(IORE_MSG_FMT, ...) do {		\
    fprintf(stdout, IORE_MSG_FMT, __VA_ARGS__); \
    fflush(stdout);				\
  } while(0)

#endif /* _UTIL_H */
