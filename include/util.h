#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mpi.h>

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define MASTER_RANK 0 /* MPI master rank */
#define MAX_RETRIES 10000 /* max number of retries for POSIX I/O */
#define MAX_STR_LEN 1024 /* max length of a string inside the program */
#define KIBIBYTE (1 << 10) /* 1 KiB */
#define MEBIBYTE (1 << 20) /* 1 MiB */
#define GIBIBYTE (1 << 30) /* 1 GiB */
#define TEBIBYTE (1L << 40) /* 1 TiB */

/* enumeration of supported verbosity levels */
typedef enum verbosity
  {
    QUIET, NORMAL, VERBOSE, VERY_VERBOSE, DEBUG
  } verbosity_t;

/* enumeration of file sharing policies */
typedef enum sharing_policy
  {
    SHARED_FILE, FILE_PER_PROCESS
  } sharing_policy_t;

/* enumeration of access pattern in terms of file offsets */
typedef enum access_pattern
  {
    SEQUENTIAL, RANDOM
  } access_pattern_t;

/* enumeration of supported access type tests */
typedef enum access
  {
    READ, WRITE
  } access_t;

typedef double iore_time_t; /* execution time */
typedef long long int iore_offset_t; /* file offset */
typedef long long int iore_size_t; /* sizes in byte units or multiples */

/* enumeration of collected timers; W for write, R for read, and D for delete */
enum timer
  {
    W_OPEN_START,
    W_OPEN_STOP,
    W_START,
    W_STOP,
    W_CLOSE_START,
    W_CLOSE_STOP,
    R_OPEN_START,
    R_OPEN_STOP,
    R_START,
    R_STOP,
    R_CLOSE_START,
    R_CLOSE_STOP,
    D_START,
    D_STOP,
    NUM_TIMERS
  };

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

char *current_time_str();
iore_time_t current_time();
void sync_rand_gen(MPI_Comm);
char *human_readable(iore_size_t);
char *get_parent_path(char *);

/******************************************************************************
 * M A C R O S
 ******************************************************************************/

/*
 * Compares two strings for equality.
 */
#define STREQUAL(A, B) (strcmp(A, B) == 0)

/*
 * Prepare a custom formatted message.
 */
#define FMTMSG(FMT, ...)			\
  char msg[MAX_STR_LEN];			\
  sprintf(msg, FMT, __VA_ARGS__);		\

/*
 * Display a fatal pre-formatted error message.
 */
#define FATAL(MSG)						\
  do {								\
    fprintf(stderr, "IORE FATAL: %s, errno %d, %s (%s:%d).\n",	\
	    MSG, errno, strerror(errno), __FILE__, __LINE__);	\
    fflush(stderr);						\
  } while(0)

/*
 * Format and display a fatal message.
 */
#define FATALF(FMT, ...)					\
  do {								\
    FMTMSG(FMT, __VA_ARGS__);					\
    fprintf(stderr, "IORE FATAL: %s, errno %d, %s (%s:%d).\n",	\
	    msg, errno, strerror(errno), __FILE__, __LINE__);	\
    fflush(stderr);						\
  } while(0)

/*
 * Display a pre-formatted error message.
 */
#define ERR(MSG)				\
  do {						\
    fprintf(stderr, "IORE ERROR: %s.\n", MSG);	\
    fflush(stderr);				\
  } while(0)

/*
 * Format and display an error message.
 */
#define ERRF(FMT, ...)						\
  do {								\
    FMTMSG(FMT, __VA_ARGS__);					\
    fprintf(stderr, "IORE ERROR: %s.\n", msg);			\
    fflush(stderr);						\
  } while(0)

/*
 * Displays a pre-formatted warning message.
 */
#define WARN(MSG)					\
  do {							\
    fprintf(stderr, "IORE WARNING: %s.\n\n", MSG);	\
    fflush(stderr);					\
  } while(0)

/*
 * Formats and displays a warning message.
 */
#define WARNF(FMT, ...)				 \
  do {						 \
    FMTMSG(FMT, __VA_ARGS__);			 \
    fprintf(stderr, "IORE WARNING: %s.\n\n", msg); \
    fflush(stderr);				 \
  } while(0)

/*
 * Checks the return code of an MPI fuction. If an error occurs, displays a
 * combined custom and error message, and aborts the program execution.
 */
#define MPI_TRYCATCH(ERRCODE, MSG)					\
  do {									\
    if (ERRCODE != MPI_SUCCESS)						\
      {									\
	char errstr[MPI_MAX_ERROR_STRING];				\
	int errstrlen;							\
									\
	MPI_Error_string(ERRCODE, errstr, &errstrlen);			\
	fprintf(stderr, "IORE FATAL: %s, MPI %s, (%s:%d)\n", MSG,	\
		errstr, __FILE__, __LINE__);				\
	fflush(stderr);							\
	MPI_Abort(MPI_COMM_WORLD, -1);					\
      }									\
  } while(0)

#endif /* _UTIL_H */
