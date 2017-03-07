/******************************************************************************\
 * IORE - The IOR-Extended Benchmark
 * 
 * Author: Camilo <eduardo.camilo@posgrad.ufsc.br>
 * Creation date: 2017-03-07
 * License: GPLv3
 * 
 * Distributed Systems Research Laboratory (LAPESD)
 * Federal University of Santa Catarina (UFSC)
\******************************************************************************/

#ifndef _IOREDEF_H
#define _IOREDEF_H

/* TODO: check the need for this code snippet */
/* 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mpi.h>

/* TODO: check the need for these includes and _WIN32 check */
/*
#include <sys/param.h>                               
#include <unistd.h>
#include <limits.h>
 */

/**** DECLARATIONS ****/ 
/* TODO: check the need for these declarations */
/*
extern int numTasks;                           
extern int rank;
extern int rankOffset;
extern int verbose;                            
*/ 

/* TODO: should include other declarations? */

/**** DEFINITIONS ****/
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/* TODO: check MEGABYTE in ior.c lines 1785 and 1789 */
/*
#define MEGABYTE 1000000
*/
#define KIBIBYTE (1 << 10)
#define MEBIBYTE (1 << 20)
#define GIBIBYTE (1 << 30)

/* operation modes */
#define WRITE 0
#define WRITECHECK 1
#define READ 2
#define READCHECK 3
#define CHECK 4

/* verbosity settings */
#define VERBOSE_0 0
#define VERBOSE_1 1
#define VERBOSE_2 2
#define VERBOSE_3 3
#define VERBOSE_4 4
#define VERBOSE_5 5

/* TODO: check the need for these definitions */
/*
#define MAX_HINTS 16
*/
#define MAX_STR 1024
#define MAX_RETRY 10000
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define FILENAME_DELIMITER '@'

/**** CUSTOM TYPES ****/
typedef long long int IORE_offset_t;
typedef long long int IORE_size_t;

/**** MACROS ****/
/* WARN_RESET will display a custom error message and set value to default */
#define WARN_RESET(MSG, TO_STRUCT_PTR, FROM_STRUCT_PTR, MEMBER) do {    \
    (TO_STRUCT_PTR)->MEMBER = (FROM_STRUCT_PTR)->MEMBER;		\
    if (rank == 0) {							\
      fprintf(stdout, "iore WARNING: %s.  Using value of %d.\n",	\
	      MSG, (TO_STRUCT_PTR)->MEMBER);				\
    }									\
    fflush(stdout);							\
  } while (0)

#define WARN(MSG) do {							\
    if (verbose > VERBOSE_2) {						\
      fprintf(stdout, "iore WARNING: %s, (%s:%d).\n",			\
	      MSG, __FILE__, __LINE__);					\
    } else {								\
      fprintf(stdout, "iore WARNING: %s.\n", MSG);			\
    }									\
    fflush(stdout);							\
  } while (0)

/* warning with errno printed */
#define EWARN(MSG) do {							\
    if (verbose > VERBOSE_2) {						\
      fprintf(stdout, "iore WARNING: %s, errno %d, %s (%s:%d).\n",	\
	      MSG, errno, strerror(errno), __FILE__, __LINE__);		\
    } else {								\
      fprintf(stdout, "iore WARNING: %s, errno %d, %s \n",		\
	      MSG, errno, strerror(errno));				\
    }									\
    fflush(stdout);							\
  } while (0)

/* display error message and terminate execution */
#define ERR(MSG) do {							\
    fprintf(stdout, "iore ERROR: %s, errno %d, %s (%s:%d)\n",		\
	    MSG, errno, strerror(errno), __FILE__, __LINE__);		\
    fflush(stdout);							\
    MPI_Abort(MPI_COMM_WORLD, -1);					\
  } while (0)

/*
 * MPI_CHECK will display a custom error message as well as an error string
 * from the MPI_STATUS and then exit the program
 */

#define MPI_CHECK(MPI_STATUS, MSG) do {					\
    char resultString[MPI_MAX_ERROR_STRING];				\
    int resultLength;							\
    									\
    if (MPI_STATUS != MPI_SUCCESS) {					\
      MPI_Error_string(MPI_STATUS, resultString, &resultLength);	\
      fprintf(stdout, "iore ERROR: %s, MPI %s, (%s:%d)\n",		\
	      MSG, resultString, __FILE__, __LINE__);			\
      fflush(stdout);							\
      MPI_Abort(MPI_COMM_WORLD, -1);					\
    }									\
  } while(0)

/* TODO: check the need for _WIN32 definitions */

#endif /* _IOREDEF_H */
