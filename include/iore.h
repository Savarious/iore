#ifndef _IORE_H
#define _IORE_H

#include "util.h"

/*****************************************************************************
 * T Y P E S   A N D   S T R U C T S                                         *
 *****************************************************************************/

typedef long long int IORE_offset_t;
typedef long long int IORE_size_t;

typedef struct {
  int use_o_direct; /* use O_DIRECT flag (bypass I/O buffers) */

  IORE_offset_t offset; /* file offset for read/write */
  
  enum VERBOSE verbose;

  /* POSIX parameters */
  int fsync; /* fsync() after writing the whole file */
  int fsync_per_write; /* fsync() after each write() */
  int single_io_attempt; /* do not retry an I/O operation if incomplete */
} IORE_param_t;

typedef struct {

} IORE_results_t;

/*
 * List of tests, with its parameters and results.
 */
typedef struct IORE_test_t {
  IORE_param_t params;
  IORE_results_t *results;
  struct IORE_test_t *next;
} IORE_test_t;

#endif /* _IORE_H */
