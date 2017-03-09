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
 * Display a custom error message, the error number, and the error string.
 *
 * IORE_MSG: custom error message
 */
#define ERR(IORE_MSG) do {						\
    fprintf(stderr, "IORE ERROR: %s, errno %d, %s (%s:%d)\n",		\
	    IORE_MSG, errno, strerror(errno), __FILE__, __LINE__);	\
    fflush(stderr);							\
  } while(0)

#endif /* _UTIL_H */
